#include "level8.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "interactionsystem.h"
#include "grid.h"

#include <QFont>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QVariantAnimation>
#include <QTimer>
#include <algorithm>

Level8::Level8(QObject *parent) : LevelBase(parent) {}

//  入场序列：从门渐显 、 走到 (15,12) 、 对话
void Level8::startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                         bool &inputEnabled) {
    if (!player || !m_dialogueSystem) return;
    inputEnabled = false;
    if (arrow) arrow->setVisible(false);

    // 找到底部"门"(15,14) 作为入场点
    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (!ti || ti == player) continue;
        if (ti->toPlainText() == QStringLiteral("门")
            && Grid::toGrid(ti->pos()) == QPoint(15, 14)) {
            m_doorItem = ti;
            break;
        }
    }

    // 收集石头（5块）
    m_stones.clear();
    const QList<QPoint> stonePositions = {{9,0}, {9,1}, {10,1}, {11,1}, {11,0}};
    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (!ti || ti == player) continue;
        if (ti->toPlainText() == QStringLiteral("石")
            && stonePositions.contains(Grid::toGrid(ti->pos()))) {
            m_stones.append(ti);
        }
    }

    // 启动环境动画
    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (!ti || ti == player) continue;
        QPoint g = Grid::toGrid(ti->pos());
        if (ti->toPlainText() == QStringLiteral("机") && g == QPoint(8, 10))
            startSparks(ti);
        if (ti->toPlainText() == QStringLiteral("人") && g == QPoint(12, 4))
            startShiver(ti);
        if (ti->toPlainText() == QStringLiteral("人") && g == QPoint(16, 6))
            startGroan(ti);
    }

    // 注册所有 F 键交互
    registerInteractions();

    // 玩家从 (15,14) 的"门"渐显
    player->setOpacity(0.0);
    player->setPos(Grid::toPixel(15, 14));

    Anim::fade(this, player, 1.0, 600, [=, &inputEnabled]() {
        Anim::slide(this, player, Grid::toPixel(15, 12), 1000, [=, &inputEnabled]() {
            inputEnabled = true;
            if (arrow) {
                QPointF pp = player->pos();
                arrow->setPos(pp.x() + Grid::kSize, pp.y());
                arrow->setPlainText(QStringLiteral("▶"));
                arrow->setVisible(true);
            }
            QStringList lines = {
                QStringLiteral("我：怎么回事，这里一片狼藉!"),
                QStringLiteral("尚且清醒的同学：是...恶龙干的..."),
                QStringLiteral("我们几个拼命阻止，但马上就要拦不住它了..."),
                QStringLiteral("你要挑战恶龙吗？千万当心......!!")
            };
            m_dialogueSystem->showSequence(lines, [=]() {
                emit entryComplete();
            });
        }, QEasingCurve::InOutCubic);
    });
}

//  注册 F 键交互
void Level8::registerInteractions() {
    if (!m_interactionSystem) return;

    // 1. 纸 (3,8)
    m_interactionSystem->registerInteraction({
        {3, 8}, {
            {QStringLiteral("一张放在桌子上的纸。"), {}},
            {QStringLiteral("纸上写着：鍒兂澶锛屽悜鍓嶅啿鍚"), {}},
            {QStringLiteral("..."), {}},
            {QStringLiteral(".................."), {}},
            {QStringLiteral("何意味。"), {}}
        }
    });

    // 2. 机 (8,10)
    m_interactionSystem->registerInteraction({
        {8, 10}, {
            {QStringLiteral("少数没有被灾难完全破坏的计算机，"), {}},
            {QStringLiteral("屏幕上显示着某位不幸的同学写到一半的代码。"), {}},
            {QStringLiteral("#include <iostream>"), {}, true},
            {QStringLiteral("using namespace std;"), {}, true},
            {QStringLiteral("int main(){"), {}, true},
            {QStringLiteral("cout<<\"Bye,world!\"<<endl;"), {}, true},
            {QStringLiteral("return 0;}"), {}, true},
            {QStringLiteral("......"), {}},
            {QStringLiteral("你默默地关掉了这台计算机。"), {}}
        }
    });

    // 3. 人 (15,11)
    m_interactionSystem->registerInteraction({
        {15, 11}, {
            {QStringLiteral("加油，你是我们最后的希望..."), {}}
        }
    });

    // 4. 人 (16,6)
    m_interactionSystem->registerInteraction({
        {16, 6}, {
            {QStringLiteral("负伤的同学：好多弹幕！"), {}},
            {QStringLiteral("好多好多好多好多好多好多弹幕！"), {}},
            {QStringLiteral("我要折在这里了吗呜呜呜呜呜呜呜......"), {}}
        }
    });

    // 5. 人 (12,4)
    m_interactionSystem->registerInteraction({
        {12, 4}, {
            {QStringLiteral("（他似乎已经处于半昏迷状态了...）"), {}},
            {QStringLiteral("朗..."), {}},
            {QStringLiteral("我：什么？我听不清..."), {}},
            {QStringLiteral("同学：□□□对□□的恐□□"), {}},
            {QStringLiteral("□□了□□□的□生..."), {}},
            {QStringLiteral("我：似乎是阴暗沉重的秘密..."), {}}
        }
    });

    // 6. 机 (17,2)
    m_interactionSystem->registerInteraction({
        {17, 2}, {
            {QStringLiteral("破烂不堪的计算机。"), {}},
            {QStringLiteral("或许是因为离恶龙太近，所以受到了波及。"), {}}
        }
    });

    // 7. 石 (any of 5 positions) — "找不到方法将它们打碎" 中 "不" 仅可删除，不可推动
    InteractionLine stoneLine1{QStringLiteral("沉重的石头。"), {}};
    InteractionLine stoneLine2{QStringLiteral("找不到方法将它们打碎"), {}}; // "不" 只能被 Backspace 删除
    InteractionLine stoneLine3{QStringLiteral("到底怎么办呢？"), {}};
    QList<InteractionLine> stoneLines = {stoneLine1, stoneLine2, stoneLine3};

    const QList<QPoint> stonePositions = {{9,0}, {9,1}, {10,1}, {11,1}, {11,0}};
    for (const QPoint &p : stonePositions) {
        Interaction inter;
        inter.gridPos = p;
        inter.lines = stoneLines;
        m_interactionSystem->registerInteraction(inter);
    }
}

//  环境动画
void Level8::startSparks(QGraphicsTextItem *computer) {
    if (!computer || !m_scene) return;

    auto makeSpark = [this, computer](const QString &text, qreal ox, qreal oy, QColor color) {
        QFont f(QStringLiteral("黑体"), 11);
        auto *s = m_scene->addText(text, f);
        s->setDefaultTextColor(color);
        s->setPos(computer->pos() + QPointF(ox, oy));
        s->setZValue(50);
        m_ambientItems.append(s);

        auto *timer = new QTimer(this);
        int *tick = new int(0);
        connect(timer, &QTimer::timeout, this, [s, tick]() mutable {
            if (!s || !s->scene()) return;
            (*tick)++;
            qreal a = 0.4 + 0.6 * qAbs(std::sin((*tick) * 0.25));
            s->setOpacity(a);
        });
        timer->start(60);
        m_ambientTimers.append(timer);
    };

    // 右上角 "火星" 橙红闪烁
    makeSpark(QStringLiteral("火"), 30, -18, QColor(255, 140, 30));
    makeSpark(QStringLiteral("星"), 48, -18, QColor(255, 100, 20));
    // 左下角 "噼啪"
    makeSpark(QStringLiteral("噼"), -4, 30, QColor(255, 160, 60));
    makeSpark(QStringLiteral("啪"), 14, 30, QColor(255, 130, 40));
}

void Level8::startShiver(QGraphicsTextItem *person) {
    if (!person || !m_scene) return;

    QFont f(QStringLiteral("黑体"), 12);
    auto *s = m_scene->addText(QStringLiteral("嘶..."), f);
    s->setDefaultTextColor(QColor(220, 220, 220));
    s->setPos(person->pos() + QPointF(person->boundingRect().width() + 4, -4));
    s->setZValue(50);
    m_ambientItems.append(s);

    auto *timer = new QTimer(this);
    int *tick = new int(0);
    connect(timer, &QTimer::timeout, this, [s, tick]() mutable {
        if (!s || !s->scene()) return;
        (*tick)++;
        qreal ox = 2.0 * std::sin((*tick) * 0.8);
        qreal oy = 1.5 * std::cos((*tick) * 1.1);
        QPointF base = s->data(0).toPointF();
        s->setPos(base + QPointF(ox, oy));
    });
    s->setData(0, s->pos());
    timer->start(35);
    m_ambientTimers.append(timer);
}

void Level8::startGroan(QGraphicsTextItem *person) {
    if (!person || !m_scene) return;

    QFont f(QStringLiteral("黑体"), 12);
    auto *s = m_scene->addText(QStringLiteral("额啊..."), f);
    s->setDefaultTextColor(QColor(200, 200, 200));
    s->setPos(person->pos() + QPointF(person->boundingRect().width() + 4, -4));
    s->setTransformOriginPoint(s->boundingRect().center());
    s->setZValue(50);
    m_ambientItems.append(s);

    auto *sway = Anim::sway(this, s, 4.0, 1800);
    if (sway) m_ambientAnims.append(sway);
}

//  石头破碎检测
bool Level8::tryStoneBreak(QList<QGraphicsTextItem*> &pushables,
                            QList<QGraphicsTextItem*> &waters,
                            bool &inputEnabled,
                            QGraphicsTextItem *player) {
    if (m_stonesBroken || !m_interactionSystem || !m_interactionSystem->isActive())
        return false;

    // 只在石头交互中生效
    QPoint g = m_interactionSystem->currentGridPos();
    bool isStone = false;
    const QList<QPoint> sps = {{9,0}, {9,1}, {10,1}, {11,1}, {11,0}};
    for (const QPoint &p : sps) { if (g == p) { isStone = true; break; } }
    if (!isStone) return false;

    // 直接用 InteractionSystem 的精确检测
    QString visible = m_interactionSystem->currentVisibleText();
    if (visible != QStringLiteral("找到方法将它们打碎")) return false;

    //石头破碎
    m_stonesBroken = true;

    QList<QPointer<QGraphicsTextItem>> bangItems;
    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (!ti || ti->toPlainText() != QStringLiteral("石")) continue;
        QFont bangFont(QStringLiteral("黑体"), 18);
        auto *bang = m_scene->addText(QStringLiteral("轰！"), bangFont);
        bang->setDefaultTextColor(Qt::white);
        bang->setPos(ti->pos() + QPointF(Grid::kSize, -8));
        bang->setZValue(90);
        m_ambientItems.append(bang);
        bangItems.append(bang);
    }

    // 2 秒后渐隐"轰！"特效
    QTimer::singleShot(2000, this, [this, bangItems]() {
        for (auto &bp : bangItems) {
            if (!bp) continue;
            Anim::fade(this, bp.data(), 0.0, 600, [this, bp]() {
                if (bp) {
                    m_ambientItems.removeAll(bp.data());
                    if (bp->scene()) bp->scene()->removeItem(bp.data());
                    delete bp.data();
                }
            });
        }
    });

    QList<QPropertyAnimation*> stoneAnims;
    for (auto *stone : m_stones) {
        if (!stone) continue;
        stone->setGraphicsEffect(nullptr);
        stoneAnims.append(Anim::prop(this, stone, "opacity", 800, 0.0));
    }

    Anim::parallel(this, stoneAnims, [this, &pushables, &waters, &inputEnabled]() {
        for (auto *stone : m_stones) {
            if (stone) {
                pushables.removeAll(stone);
                waters.removeAll(stone);
                if (stone->scene()) stone->scene()->removeItem(stone);
                delete stone;
            }
        }
        m_stones.clear();
        if (m_interactionSystem) m_interactionSystem->cleanup(pushables, waters);
        emit stonesBroken();
    });

    return true;
}

//  清理
void Level8::cleanup() {
    for (auto *a : m_ambientAnims) { a->stop(); delete a; }
    m_ambientAnims.clear();
    for (auto *t : m_ambientTimers) { t->stop(); delete t; }
    m_ambientTimers.clear();
    for (auto *item : m_ambientItems) {
        if (item && item->scene()) { item->scene()->removeItem(item); delete item; }
    }
    m_ambientItems.clear();
    m_stones.clear();
    m_stonesBroken = false;
    m_doorItem = nullptr;

    LevelBase::cleanup();
}
