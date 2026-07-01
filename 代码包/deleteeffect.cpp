#include "deleteeffect.h"
#include "animutils.h"
#include "grid.h"

#include <QFont>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>

DeleteEffect::DeleteEffect(QObject *parent)
    : QObject(parent)
{
}

void DeleteEffect::play(QGraphicsTextItem *source, QGraphicsTextItem *target,
                         std::function<void()> onDone) {
    if (!m_scene || !source || !target || m_playing) return;
    m_playing = true;

    //创建 "←" 图标
    QFont arrowFont("黑体", 28, QFont::Bold);
    QGraphicsTextItem *arrow = m_scene->addText("←", arrowFont);
    arrow->setDefaultTextColor(QColor(255, 255, 255));  // 白色
    arrow->setZValue(50);

    // 放在 source 的正上方一格处
    QPointF srcPos = source->pos();
    arrow->setPos(srcPos.x(), srcPos.y() - Grid::kSize);
    arrow->setTransformOriginPoint(arrow->boundingRect().center());
    arrow->setScale(0.3);
    arrow->setOpacity(0.0);

    // "←" 放大 + 渐显 (0~250ms)，然后渐隐
    Anim::parallel(this, {
        Anim::prop(this, arrow, "scale", 250, 2.0),
        Anim::prop(this, arrow, "opacity", 250, 1.0)
    }, [this, arrow, target, onDone]() {
        Anim::fade(this, arrow, 0.0, 250, nullptr, QEasingCurve::InCubic);
        Anim::fade(this, target, 0.0, 500, [this, arrow, target, onDone]() {
            m_scene->removeItem(arrow);
            delete arrow;
            m_scene->removeItem(target);
            delete target;
            m_playing = false;
            if (onDone) onDone();
        }, QEasingCurve::InCubic);
    });
}

//  Enter之盾特效：↵ 渐显放大 、 渐隐
//  使用 QVariantAnimation 手动驱动（避免 QPropertyAnimation 直接
//  操作 QGraphicsItem 在特定时机的不稳定性）
void DeleteEffect::playEnterShield(QGraphicsTextItem *player) {
    if (!m_scene || !player || m_playing) return;
    m_playing = true;

    QFont symFont(QStringLiteral("黑体"), 28, QFont::Bold);
    auto *sym = m_scene->addText(QStringLiteral("↵"), symFont);
    sym->setDefaultTextColor(QColor(100, 255, 200));
    sym->setZValue(60);

    // 放在 player 正上方一格
    QPointF pp = player->pos();
    sym->setPos(pp.x(), pp.y() - Grid::kSize);
    sym->setTransformOriginPoint(sym->boundingRect().center());
    sym->setScale(0.3);
    sym->setOpacity(0.0);

    // 阶段1：放大 + 渐显 (0~250ms)
    Anim::variantAnim(this, 0.0, 1.0, 250, QEasingCurve::OutCubic,
        [sym](qreal t) {
            if (!sym || !sym->scene()) return;
            sym->setScale(0.3 + 1.7 * t);
            sym->setOpacity(t);
        },
        [this, sym]() {
            // 阶段2：渐隐 (250~500ms)
            Anim::variantAnim(this, 1.0, 0.0, 250, QEasingCurve::InCubic,
                [sym](qreal v) { if (sym && sym->scene()) sym->setOpacity(v); },
                [this, sym]() {
                    if (sym && sym->scene()) sym->scene()->removeItem(sym);
                    delete sym;
                    m_playing = false;
                });
        });
}
