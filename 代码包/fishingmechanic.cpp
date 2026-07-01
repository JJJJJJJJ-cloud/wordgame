#include "fishingmechanic.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "grid.h"
#include "achievement.h"

#include <QFont>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QAbstractAnimation>
#include <QPointer>
#include <cstdlib>
#include <cmath>

FishingMechanic::FishingMechanic(QObject *parent)
    : QObject(parent)
{
}

// 第五关入场叙事
void FishingMechanic::startLevel5Entry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                                        bool &inputEnabled, bool &arrowEnabled,
                                        const QStringList &fishingItems, int fishingTargetIndex)
{
    if (!m_scene || !player) return;

    arrowEnabled = false;
    if (arrow) arrow->setVisible(false);
    inputEnabled = false;

    // 找老师
    QGraphicsTextItem *teacher = nullptr;
    for (auto *item : m_scene->items()) {
        if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
            if (ti->toPlainText() == QStringLiteral("师")) {
                teacher = ti;
                break;
            }
        }
    }

    if (!teacher) {
        // 无老师时直接开始钓鱼
        setup(player, fishingItems, fishingTargetIndex);
        inputEnabled = true;
        return;
    }

    // 老师 + 主角从屏幕外滑入
    QPointer<QGraphicsTextItem> tp(teacher);
    QPointer<QGraphicsTextItem> pp(player);
    auto *ds = m_dialogueSystem;  // 捕获指针

    Anim::parallel(m_scene, {
        Anim::prop(m_scene, teacher, "pos", 600, Grid::toPixel(6, 0)),
        Anim::prop(m_scene, player, "pos", 600, Grid::toPixel(7, 0))
    }, [=, &inputEnabled]() {
        if (!pp || !tp) return;
        if (!ds) return;
        ds->setSequencePos(1, 2, 16);

        QStringList lines = {
            QStringLiteral("老师：事发突然，再加上看到了百年难遇的太以编辑能力者，"),
            QStringLiteral("我太激动了，忘记向你解释现状。"),
            QStringLiteral("前些天，有另一位和你能力相近的人留下预言："),
            QStringLiteral("恶龙将会降世，需太以编辑与之对抗。"),
            QStringLiteral("命定的勇者，携神器两件，惩恶扬善。"),
            QStringLiteral("其一，为贝克思贝斯之剑，"),
            QStringLiteral("其二，为恩特之盾。"),
            QStringLiteral("有此神器，便可所向披靡。"),
            QStringLiteral("据说神器之一，贝克思贝斯之剑沉于未名湖底。"),
            QStringLiteral("抓紧把它钓上来吧！"),
            QStringLiteral("我：好的，我这就..."),
            QStringLiteral("...老师，您说\"钓\"上来！？"),
            QStringLiteral("老师：嗯嗯，怎么了？"),
            QStringLiteral("我：（算了，发生什么离奇的事我也习惯了）"),
            QStringLiteral("（提示：按AD左右移动，按空格键放下绳子垂钓）")
        };

        ds->showSequence(lines, [=, &inputEnabled]() {
            if (!pp) return;
            // 老师淡出
            if (tp) {
                QPointer<QGraphicsTextItem> tpp(tp);
                Anim::fade(m_scene, tp.data(), 0.0, 400, [tpp]() {
                    if (tpp) { tpp->scene()->removeItem(tpp.data()); delete tpp.data(); }
                });
            }

            // 主角走到钓鱼位并启动钓鱼
            Anim::slide(m_scene, player, Grid::toPixel(10, 0), 400, [=, &inputEnabled]() {
                setup(player, fishingItems, fishingTargetIndex);
                inputEnabled = true;
            });
        });
    });
}

// 初始化
void FishingMechanic::setup(QGraphicsTextItem *player,
                              const QStringList &fishingItems,
                              int fishingTargetIndex) {
    m_fishingActive = false;
    m_ropeY = 0;
    m_targetIndex = fishingTargetIndex;
    m_fishingItemNames = fishingItems;
    m_targets.clear();
    m_duck = nullptr;

    // 收集湖底物品；水面上的鸭子单独处理（不进 m_targets）
    for (auto *item : m_scene->items()) {
        if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
            if (ti == player) continue;
            if (ti->toPlainText() == "鸭子") { m_duck = ti; continue; }
            QPoint g = Grid::toGrid(ti->pos());
            if (g.y() >= 12) m_targets.append(ti);
        }
    }

    // 被钓元素的字号
    QFont itemFont("黑体", 18);
    for (auto *t : m_targets) t->setFont(itemFont);
    if (m_duck) m_duck->setFont(itemFont);

    startRandomMovement(player);
    if (m_duck) startDuckMovement();
}

void FishingMechanic::startRandomMovement(QGraphicsTextItem *player) {
    m_moveTimer = new QTimer(this);
    connect(m_moveTimer, &QTimer::timeout, this, [this, player]() {
        if (m_fishingActive) return;

        // 为每个物品挑选互不重叠的落点（移动途中可能交叉，但停下后不重合）
        struct Span { int row, x0, x1; };
        QList<Span> reserved;
        const int kGap = 1;                // 物品间额外留 1 格空隙
        const int kMinX = 1, kMaxX = 18;

        auto widthInCells = [](QGraphicsTextItem *t) {
            int w = static_cast<int>(std::ceil(t->boundingRect().width() / Grid::kSize));
            return w < 1 ? 1 : w;
        };
        auto overlaps = [&](int row, int x0, int x1) {
            for (const Span &s : reserved) {
                if (s.row != row) continue;
                if (x0 - kGap <= s.x1 && x1 + kGap >= s.x0) return true;
            }
            return false;
        };

        for (auto *t : m_targets) {
            if (!t) continue;
            int w = widthInCells(t);
            int hi = kMaxX - (w - 1);       // 保证整体不越界
            if (hi < kMinX) hi = kMinX;

            int nx = -1, ny = 12;
            for (int attempt = 0; attempt < 40; ++attempt) {
                int cx = kMinX + (std::rand() % (hi - kMinX + 1));
                int cy = 12 + (std::rand() % 3);
                if (!overlaps(cy, cx, cx + w - 1)) { nx = cx; ny = cy; break; }
            }
            if (nx < 0) {                   // 找不到空位则保持当前格
                QPoint g = Grid::toGrid(t->pos());
                nx = g.x(); ny = g.y();
            }
            reserved.append({ny, nx, nx + w - 1});

            QPointF target = Grid::toPixel(nx, ny);
            auto *anim = new QPropertyAnimation(t, "pos");
            anim->setDuration(600 + std::rand() % 600);
            anim->setEndValue(target);
            anim->setEasingCurve(QEasingCurve::Linear);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
        m_moveTimer->setInterval(800 + std::rand() % 1200);
    });
    m_moveTimer->start(1000);
}

// 鸭子：只左右移动
void FishingMechanic::startDuckMovement() {
    m_duckTimer = new QTimer(this);
    connect(m_duckTimer, &QTimer::timeout, this, [this]() {
        if (!m_duck || m_fishingActive) return;
        int row = Grid::toGrid(m_duck->pos()).y();   // 保持当前行（水面），只改 x
        int nx = 1 + (std::rand() % 17);
        QPointF target = Grid::toPixel(nx, row);
        auto *anim = new QPropertyAnimation(m_duck, "pos");
        anim->setDuration(700 + std::rand() % 600);
        anim->setEndValue(target);
        anim->setEasingCurve(QEasingCurve::Linear);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        m_duckTimer->setInterval(900 + std::rand() % 1000);
    });
    m_duckTimer->start(1000);
}

// 放绳
void FishingMechanic::dropRope(QGraphicsTextItem *player, bool inputEnabled, bool &inputEnabledRef) {
    Q_UNUSED(inputEnabled);
    if (m_fishingActive || !player || !inputEnabledRef) return;

    m_fishingActive = true;
    m_ropeY = 1;

    if (m_dialogueSystem) m_dialogueSystem->cleanup();

    if (m_fishingTimer) { m_fishingTimer->stop(); delete m_fishingTimer; }
    m_fishingTimer = new QTimer(this);
    connect(m_fishingTimer, &QTimer::timeout, this, [this, player, &inputEnabledRef]() {
        tick(player, inputEnabledRef);
    });
    m_fishingTimer->start(80);
}

// 逐格下落
void FishingMechanic::tick(QGraphicsTextItem *player, bool &inputEnabledRef) {
    if (!player) { cleanup(); return; }
    int ropeX = Grid::toGrid(player->pos()).x();

    QFont f("黑体", 12);
    QGraphicsTextItem *seg = m_scene->addText("绳", f);
    seg->setDefaultTextColor(Qt::white);
    seg->setPos(Grid::toPixel(ropeX, m_ropeY));
    m_ropeSegments.append(seg);

    // 在两行之间补一个“绳”字，缩小上下两段的间距，使绳子更连贯
    if (m_ropeY >= 1) {
        QPointF base = Grid::toPixel(ropeX, m_ropeY);
        QGraphicsTextItem *filler = m_scene->addText("绳", f);
        filler->setDefaultTextColor(Qt::white);
        filler->setPos(base.x(), base.y() - Grid::kSize * 0.5);
        m_ropeSegments.append(filler);
    }

    QGraphicsTextItem *hitItem = nullptr;
    // 绳子只要碰到元素的任意位置（整体外接矩形相交）就算钓上
    QRectF segRect = seg->sceneBoundingRect();
    for (auto *t : m_targets) {
        if (!t || t == player || m_ropeSegments.contains(t)) continue;
        if (segRect.intersects(t->sceneBoundingRect())) {
            hitItem = t;
            break;
        }
    }
    // 绳子下落途中经过水面时也可能钓到鸭子
    if (!hitItem && m_duck && segRect.intersects(m_duck->sceneBoundingRect()))
        hitItem = m_duck;

    if (hitItem) {
        QString hitText = hitItem->toPlainText();
        finishRound();

        bool isTarget = (m_fishingItemNames.indexOf(hitText) == m_targetIndex);

        // 钓上来的元素消失
        m_targets.removeOne(hitItem);
        bool isDuck = (hitItem == m_duck);
        if (isDuck) m_duck = nullptr;
        QPointer<QGraphicsTextItem> leaving = hitItem;

        if (isDuck) {
            // 鸭子被钓上后飞走：朝离它较远的那侧、向上飞出屏幕外较远处
            qreal sceneW = m_scene->sceneRect().width();
            qreal curX = hitItem->pos().x();
            qreal endX = (curX < sceneW / 2) ? sceneW + 300 : -300;   // 飞向较远的一侧并飞出边缘
            QPointF endPos(endX, -300);                               // 同时向上飞离屏幕
            auto *fly = new QPropertyAnimation(hitItem, "pos");
            fly->setDuration(2600);
            fly->setEndValue(endPos);
            fly->setEasingCurve(QEasingCurve::InQuad);
            connect(fly, &QPropertyAnimation::finished, this, [this, leaving]() {
                if (leaving) { m_scene->removeItem(leaving); delete leaving; }
            });
            fly->start(QAbstractAnimation::DeleteWhenStopped);
        } else {
            // 其它元素淡出消失
            auto *fade = new QPropertyAnimation(hitItem, "opacity");
            fade->setDuration(500);
            fade->setEndValue(0.0);
            connect(fade, &QPropertyAnimation::finished, this, [this, leaving]() {
                if (leaving) { m_scene->removeItem(leaving); delete leaving; }
            });
            fade->start(QAbstractAnimation::DeleteWhenStopped);
        }

        QString catchMsg;
        if (hitText == "贝克思贝斯之剑")
            catchMsg = "贝克思贝斯之剑！终于找到了！";
        else if (hitText == "手机")
            catchMsg = "一部手机...先放到燕园大厦保卫部吧...";
        else if (hitText == "同学")
            catchMsg = "你好...？";
        else if (hitText == "共享单车")
            catchMsg = "！？车车？！";
        else if (hitText == "鸭子")
            catchMsg = "嘎嘎嘎！";
        else
            catchMsg = "钓到了" + hitText + "，不是这个...";

        if (m_dialogueSystem) {
            if (isTarget) {
                // 钓到目标剑——保持原行为
                m_dialogueSystem->display(catchMsg, 1, 2, 15, [this]() {
                    emit catchComplete(true);
                });
            } else {
                // 各物品的专属提示词（作为第二句单独弹出）
                QString hint;
                if (hitText == "鸭子")         hint = "额。";
                else if (hitText == "手机")     hint = "为什么已经见怪不怪了呢。";
                else if (hitText == "同学")     hint = "保留节目（大雾）...";
                else if (hitText == "共享单车") hint = "不敢笑，真的在树洞上见过掉湖里的车...";

                // 若湖里只剩贝克思贝斯之剑，再追加一句
                bool onlySwordLeft = (m_duck == nullptr) && (m_targets.size() == 1)
                    && m_targets.first() && m_targets.first()->toPlainText() == "贝克思贝斯之剑";

                m_dialogueSystem->display(catchMsg, 1, 2, 15, [this, hint, onlySwordLeft]() {
                    if (onlySwordLeft) {
                        // 只剩剑：不打印该物品的提示，直接说这句
                        AchievementManager::instance()->unlock(AchievementId::HaoQiXin);
                        m_dialogueSystem->display("这么久过去了，总该钓上剑了吧...", 1, 2, 15, [this]() {
                            emit catchComplete(false);
                        });
                    } else if (!hint.isEmpty())
                        m_dialogueSystem->display(hint, 1, 2, 15, [this]() {
                            emit catchComplete(false);
                        });
                    else
                        emit catchComplete(false);
                });
            }
        }
        return;
    }

    if (m_ropeY >= 14) {
        finishRound();
        if (m_dialogueSystem) {
            m_dialogueSystem->display("什么都没钓到...再试试！", 1, 2, 16, [this]() {
                retract();
            });
        }
        return;
    }
    m_ropeY++;
}

// 回收
void FishingMechanic::retract() {
    if (m_ropeSegments.isEmpty()) {
        m_fishingActive = false;
        return;
    }
    auto *fadeGroup = new QParallelAnimationGroup;
    for (auto *seg : m_ropeSegments) {
        auto *anim = new QPropertyAnimation(seg, "opacity");
        anim->setDuration(300);
        anim->setEndValue(0.0);
        fadeGroup->addAnimation(anim);
    }
    connect(fadeGroup, &QParallelAnimationGroup::finished, this, [this]() {
        cleanupRope();
        m_fishingActive = false;
    });
    fadeGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

void FishingMechanic::cleanupRope() {
    for (auto *seg : m_ropeSegments) { m_scene->removeItem(seg); delete seg; }
    m_ropeSegments.clear();
}

// 结束本轮（保留目标和移动定时器）
void FishingMechanic::finishRound() {
    if (m_fishingTimer) { m_fishingTimer->stop(); delete m_fishingTimer; m_fishingTimer = nullptr; }
    cleanupRope();
    // 不删 m_moveTimer，不清理 m_targets ← 关键修复
}

// 关卡清理
void FishingMechanic::cleanup() {
    if (m_fishingTimer) { m_fishingTimer->stop(); delete m_fishingTimer; m_fishingTimer = nullptr; }
    if (m_moveTimer) { m_moveTimer->stop(); delete m_moveTimer; m_moveTimer = nullptr; }
    if (m_duckTimer) { m_duckTimer->stop(); delete m_duckTimer; m_duckTimer = nullptr; }
    cleanupRope();
    m_targets.clear();
    m_duck = nullptr;
    m_fishingActive = false;
}
