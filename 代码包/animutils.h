#ifndef ANIMUTILS_H
#define ANIMUTILS_H

#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QPointer>
#include <functional>

namespace Anim {

// 通用 property 动画
// NOTE: target 声明为 QObject*（非 QGraphicsItem*），
// 因为 QPropertyAnimation 构造函数要求 QObject* 目标。
// QGraphicsTextItem 通过 QGraphicsObject 、 QObject 继承链可隐式传入。
inline QPropertyAnimation* prop(QObject *parent, QObject *target,
                                const char *property, int ms,
                                const QVariant &endValue,
                                QEasingCurve curve = QEasingCurve::OutCubic)
{
    auto *a = new QPropertyAnimation(target, property, parent);
    a->setDuration(ms);
    a->setEndValue(endValue);
    a->setEasingCurve(curve);
    return a;
}

// 淡入淡出
inline QPropertyAnimation* fade(QObject *parent, QObject *target,
                                qreal to, int ms = 400,
                                std::function<void()> onDone = nullptr,
                                QEasingCurve curve = QEasingCurve::OutCubic)
{
    auto *a = prop(parent, target, "opacity", ms, to, curve);
    if (onDone)
        QObject::connect(a, &QPropertyAnimation::finished, parent, onDone);
    a->start(QAbstractAnimation::DeleteWhenStopped);
    return a;
}

// 滑动
inline QPropertyAnimation* slide(QObject *parent, QObject *target,
                                 const QPointF &to, int ms = 400,
                                 std::function<void()> onDone = nullptr,
                                 QEasingCurve curve = QEasingCurve::OutCubic)
{
    auto *a = prop(parent, target, "pos", ms, to, curve);
    if (onDone)
        QObject::connect(a, &QPropertyAnimation::finished, parent, onDone);
    a->start(QAbstractAnimation::DeleteWhenStopped);
    return a;
}

// 缩放
inline QPropertyAnimation* scaleTo(QObject *parent, QObject *target,
                                   qreal to, int ms = 400,
                                   std::function<void()> onDone = nullptr,
                                   QEasingCurve curve = QEasingCurve::OutCubic)
{
    auto *a = prop(parent, target, "scale", ms, to, curve);
    if (onDone)
        QObject::connect(a, &QPropertyAnimation::finished, parent, onDone);
    a->start(QAbstractAnimation::DeleteWhenStopped);
    return a;
}

// 并行组
inline QParallelAnimationGroup* parallel(QObject *parent,
    QList<QPropertyAnimation*> anims, std::function<void()> onDone = nullptr)
{
    auto *g = new QParallelAnimationGroup(parent);
    for (auto *a : anims)
        g->addAnimation(a);
    if (onDone)
        QObject::connect(g, &QParallelAnimationGroup::finished, parent, onDone);
    g->start(QAbstractAnimation::DeleteWhenStopped);
    return g;
}

// V 值动画（用于非 QGraphicsItem 属性）
inline QVariantAnimation* variantAnim(QObject *parent,
    qreal from, qreal to, int ms, QEasingCurve curve,
    std::function<void(qreal)> onValue, std::function<void()> onDone = nullptr)
{
    auto *a = new QVariantAnimation(parent);
    a->setDuration(ms);
    a->setStartValue(from);
    a->setEndValue(to);
    a->setEasingCurve(curve);
    QObject::connect(a, &QVariantAnimation::valueChanged, parent,
                     [onValue](const QVariant &v) { onValue(v.toDouble()); });
    if (onDone)
        QObject::connect(a, &QVariantAnimation::finished, parent, onDone);
    a->start(QAbstractAnimation::DeleteWhenStopped);
    return a;
}

// 黑屏转场
// onMid(overlay) 在完全黑屏时调用 —— 可在此做 loadLevel 等操作
// onMid 返回后自动淡入，完成后调用 onDone
inline void blackout(QGraphicsScene *scene, int fadeMs,
                     std::function<void(QGraphicsRectItem*)> onMid,
                     std::function<void()> onDone = nullptr)
{
    auto *overlay = scene->addRect(scene->sceneRect(),
                                   QPen(Qt::NoPen), QBrush(Qt::black));
    overlay->setOpacity(0.0);
    overlay->setZValue(999);

    auto *fadeOut = new QVariantAnimation(scene);
    fadeOut->setDuration(fadeMs);
    fadeOut->setStartValue(0.0);
    fadeOut->setEndValue(1.0);
    fadeOut->setEasingCurve(QEasingCurve::InQuad);
    QObject::connect(fadeOut, &QVariantAnimation::valueChanged, scene,
        [overlay](const QVariant &v) { overlay->setOpacity(v.toDouble()); });
    QObject::connect(fadeOut, &QVariantAnimation::finished, scene, [=]() {
        // 执行中间操作（如 loadLevel）
        if (onMid) onMid(overlay);

        // 淡入
        auto *fadeIn = new QVariantAnimation(scene);
        fadeIn->setDuration(fadeMs);
        fadeIn->setStartValue(1.0);
        fadeIn->setEndValue(0.0);
        fadeIn->setEasingCurve(QEasingCurve::OutQuad);
        QObject::connect(fadeIn, &QVariantAnimation::valueChanged, scene,
            [overlay](const QVariant &v) { overlay->setOpacity(v.toDouble()); });
        QObject::connect(fadeIn, &QVariantAnimation::finished, scene, [=]() {
            scene->removeItem(overlay);
            delete overlay;
            if (onDone) onDone();
        });
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    });
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}

// 往复式动画（ping-pong loop）
// 通用往复动画：到达终点时自动交换起点/终点并重启，形成无限循环。
// apply 回调用于将当前值施加到目标对象上。
inline QVariantAnimation* pingPong(QObject *parent,
    qreal from, qreal to, int periodMs,
    std::function<void(qreal)> apply,
    QEasingCurve curve = QEasingCurve::InOutSine)
{
    auto *a = new QVariantAnimation(parent);
    a->setDuration(periodMs);
    a->setStartValue(from);
    a->setEndValue(to);
    a->setEasingCurve(curve);
    QObject::connect(a, &QVariantAnimation::valueChanged, parent,
                     [apply](const QVariant &v) { apply(v.toDouble()); });
    QObject::connect(a, &QVariantAnimation::finished, parent, [a]() {
        qreal s = a->startValue().toDouble();
        qreal e = a->endValue().toDouble();
        a->setStartValue(e);
        a->setEndValue(s);
        a->start();
    });
    a->start();
    return a;
}

// 摇摆：对 QGraphicsTextItem 施加左右旋转的往复动画
inline QVariantAnimation* sway(QObject *parent, QGraphicsTextItem *target,
    qreal amplitude = 4.0, int periodMs = 1500)
{
    if (!target) return nullptr;
    target->setTransformOriginPoint(target->boundingRect().center());
    QPointer<QGraphicsTextItem> ptr(target);
    return pingPong(parent, -amplitude, amplitude, periodMs,
                    [ptr](qreal v) { if (ptr) ptr->setRotation(v); });
}

// 呼吸：对 QGraphicsTextItem 施加缩放的往复动画
inline QVariantAnimation* breathe(QObject *parent, QGraphicsTextItem *target,
    qreal scale = 1.08, int periodMs = 3000)
{
    if (!target) return nullptr;
    target->setTransformOriginPoint(target->boundingRect().center());
    QPointer<QGraphicsTextItem> ptr(target);
    return pingPong(parent, 1.0, scale, periodMs,
                    [ptr](qreal v) { if (ptr) ptr->setScale(v); });
}

} // namespace Anim

#endif // ANIMUTILS_H
