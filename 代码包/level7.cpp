#include "level7.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "deleteeffect.h"
#include "grid.h"
#include "achievement.h"

#include <QFont>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QVariantAnimation>
#include <QGraphicsRectItem>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <cmath>

Level7::Level7(QObject *parent)
    : LevelBase(parent)
{
    m_treeBases = {{7, 6}, {11, 2}, {16, 4}};
}

//  入场序列
void Level7::startEntry(QGraphicsTextItem *player, QGraphicsTextItem *teacher,
                         QGraphicsTextItem *arrow, bool &inputEnabled) {
    if (!player || !teacher) return;
    m_phase = Phase::Entry;
    inputEnabled = false;

    if (arrow) arrow->setVisible(false);

    player->setOpacity(0.0);
    teacher->setOpacity(0.0);

    Anim::parallel(this, {
        Anim::prop(this, player, "opacity", 800, 1.0),
        Anim::prop(this, teacher, "opacity", 800, 1.0)
    }, [=, &inputEnabled]() {
        QPointF playerTarget = Grid::toPixel(1, 4);
        QPointF teacherTarget = Grid::toPixel(2, 4);

        Anim::parallel(this, {
            Anim::prop(this, player, "pos", 700, playerTarget),
            Anim::prop(this, teacher, "pos", 700, teacherTarget)
        }, [=, &inputEnabled]() {            auto showArrow = [=]() {
                if (arrow) {
                    QPointF pp = player->pos();
                    arrow->setPos(pp.x() + Grid::kSize, pp.y());
                    arrow->setPlainText(QStringLiteral("▶"));
                    arrow->setVisible(true);
                }
            };
            if (!m_dialogueSystem) {
                inputEnabled = true;
                m_phase = Phase::FreePlay;
                showArrow();
                emit entryComplete();
                return;
            }
            QStringList lines = {
                QStringLiteral("老师：集齐了两件神器，你就是我们的希望！"),
                QStringLiteral("但是，你需要先熟悉这两件神器的用法。"),
                QStringLiteral("先从贝克思贝斯之剑开始！"),
                QStringLiteral("贝克思贝斯之剑有着危险而强大的能力，谨慎使用。"),
                QStringLiteral("它可以删去某些字，从而改变整个语义。"),
                QStringLiteral("看到那几棵树了吗？"),
                QStringLiteral("先前错过了花期，请帮我重现落英缤纷的景色吧。"),
                QStringLiteral("（提示：朝向某些特殊的字按backspace键可触发“删除”！）")
            };
            m_dialogueSystem->setSequencePos(1, 13, 18);
            m_dialogueSystem->showSequence(lines, [=, &inputEnabled]() {
                inputEnabled = true;
                m_phase = Phase::FreePlay;
                showArrow();
                emit entryComplete();
            });
        });
    });
}

//  树摇曳
void Level7::setupTrees() {
    struct TreeGroup { int gx; int gyStart, gyEnd; };
    const QList<TreeGroup> groups = {
        {7,  6, 10}, {11, 2, 6}, {16, 4, 8}
    };
    const QStringList treeChars = { QStringLiteral("没"), QStringLiteral("开"),
                                    QStringLiteral("花"), QStringLiteral("的"),
                                    QStringLiteral("树") };

    for (const auto &group : groups) {
        for (int gy = group.gyStart, ci = 0; gy <= group.gyEnd; ++gy, ++ci) {
            QPointF pixel = Grid::toPixel(group.gx, gy);
            for (auto *item : m_scene->items()) {
                auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
                if (!ti) continue;
                if (ti->pos() == pixel && ti->toPlainText() == treeChars[ci]) {
                    addTreeSway(ti);
                    break;
                }
            }
        }
    }
}

void Level7::addTreeSway(QGraphicsTextItem *tree) {
    if (!tree) return;
    auto *anim = Anim::sway(this, tree, 7.0, 2200 + (std::rand() % 600));
    if (anim) m_treeAnims.append({QPointer<QGraphicsTextItem>(tree), anim});
}

void Level7::removeTreeSway(QGraphicsTextItem *target) {
    for (int i = m_treeAnims.size() - 1; i >= 0; --i) {
        if (m_treeAnims[i].item == target) {
            if (m_treeAnims[i].anim) {
                m_treeAnims[i].anim->stop();
                m_treeAnims[i].anim->deleteLater();
            }
            m_treeAnims.removeAt(i);
        }
    }
}

//  删除检测 & 开花
bool Level7::isDeletableTreeChar(QGraphicsTextItem *item) const {
    if (!item || item->toPlainText() != QStringLiteral("没")) return false;
    if (m_phase != Phase::FreePlay) return false;

    QPoint g = Grid::toGrid(item->pos());
    for (int i = 0; i < m_treeBases.size(); ++i) {
        if (g == m_treeBases[i] && !m_bloomedFlags[i])
            return true;
    }
    return false;
}

void Level7::bloomTreeAt(QPoint gridPos, QList<QGraphicsTextItem*> &waters) {
    int groupIdx = -1;
    for (int i = 0; i < m_treeBases.size(); ++i) {
        if (m_treeBases[i] == gridPos && !m_bloomedFlags[i]) {
            groupIdx = i; break;
        }
    }
    if (groupIdx < 0) return;

    m_bloomedFlags[groupIdx] = true;
    m_bloomedCount++;

    // 清理已失效的摇曳动画
    for (int i = m_treeAnims.size() - 1; i >= 0; --i) {
        if (m_treeAnims[i].item.isNull()) {
            if (m_treeAnims[i].anim) {
                m_treeAnims[i].anim->stop();
                m_treeAnims[i].anim->deleteLater();
            }
            m_treeAnims.removeAt(i);
        }
    }

    QPoint base = m_treeBases[groupIdx];
    const QList<QPoint> flowerOffsets = {
        {-2, 1}, {-2, 2}, {-1, 1}, {-1, 0}, {0, -1},
        {0, 0}, {1, 0}, {1, 1}, {2, 1}, {2, 2}
    };

    QFont flowerFont(QStringLiteral("黑体"), 18);
    QColor pink(255, 182, 193);

    for (const QPoint &off : flowerOffsets) {
        QPoint fPos = base + off;
        auto *flower = m_scene->addText(QStringLiteral("花"), flowerFont);
        flower->setDefaultTextColor(pink);
        flower->setPos(Grid::toPixel(fPos.x(), fPos.y()));
        flower->setOpacity(0.0);
        flower->setZValue(5);
        waters.append(flower);

        Anim::fade(this, flower, 1.0, 600);

        addTreeSway(flower);
    }

    // 剩余 "开""花""的""树" 变色
    const QStringList remainChars = { QStringLiteral("开"), QStringLiteral("花"),
                                      QStringLiteral("的"), QStringLiteral("树") };
    for (int gy = base.y() + 1, ci = 0; gy <= base.y() + 4; ++gy, ++ci) {
        QPointF pixel = Grid::toPixel(base.x(), gy);
        for (auto *item : m_scene->items()) {
            auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
            if (!ti) continue;
            if (ti->pos() == pixel && ti->toPlainText() == remainChars[ci]) {
                ti->setDefaultTextColor(pink);
                break;
            }
        }
    }

    if (m_bloomedCount >= 3) {
        QTimer::singleShot(800, this, [this]() { emit allBloomed(); });
    }
}

//  Enter 键（恩特之盾规避）
void Level7::handleEnterPress() {
    // 闪避训练：弹反
    if (m_phase == Phase::DodgeTraining && m_playerPtr && m_arrowPtr && m_inputPtr) {
        QPoint playerGrid = Grid::toGrid(m_playerPtr->pos());
        QPoint ad = arrowDir(m_arrowPtr->toPlainText());
        QPoint frontCell = playerGrid + ad;

        for (int i = m_flyingBullets.size() - 1; i >= 0; --i) {
            auto &fb = m_flyingBullets[i];
            if (!fb.item) continue;
            QPoint bg = Grid::toGrid(QPointF(fb.x, fb.y));
            // 弹在箭头前方 && 弹正朝玩家飞来
            if (bg == frontCell && fb.dx == -ad.x() && fb.dy == -ad.y()) {
                // 瞬移到身后一格，方向不变
                QPoint behind = playerGrid - ad;
                fb.x = behind.x() * Grid::kSize;
                fb.y = behind.y() * Grid::kSize;
                fb.item->setPos(fb.x, fb.y);
                // 弹反成功闪光
                fb.item->setDefaultTextColor(QColor(100, 255, 200));
                if (m_deleteEffect)
                    m_deleteEffect->playEnterShield(m_playerPtr);
                return;
            }
        }
        return;
    }

    //原始弹幕阶段：规避
    if (!m_waitingForEnter) return;
    m_waitingForEnter = false;
    m_bulletDodged = true;
    if (m_deleteEffect && m_playerPtr)
        m_deleteEffect->playEnterShield(m_playerPtr);

    // 停掉闪烁提示
    if (m_enterHintTimer) { m_enterHintTimer->stop(); delete m_enterHintTimer; m_enterHintTimer = nullptr; }
    if (m_enterHint) { m_scene->removeItem(m_enterHint); delete m_enterHint; m_enterHint = nullptr; }

    // 恢复正常速度
    if (m_bulletTimer) {
        m_bulletTimer->setInterval(50);
        m_bulletTimer->start();
    }

    // y=4 的弹快速消失
    QGraphicsTextItem *midBullet = nullptr;
    for (auto *b : m_bullets) {
        if (b && Grid::toGrid(b->pos()).y() == 4) { midBullet = b; break; }
    }
    if (midBullet) {
        midBullet->hide();
        // 移到 (13,4) 重新出现
        midBullet->setPos(Grid::toPixel(13, 4));
        midBullet->setOpacity(0.0);
        Anim::fade(this, midBullet, 1.0, 100);
        midBullet->show();
    }

    m_bulletPhase = 4;  // dodged
}

//  三棵树开花后 、 转场 、 恩特之盾教学 、 弹幕
void Level7::startPostBloomSequence(QGraphicsTextItem *player,
                                     QGraphicsTextItem *teacher,
                                     QGraphicsTextItem *arrow,
                                     QList<QGraphicsTextItem*> &waters,
                                     bool &inputEnabled) {
    m_phase = Phase::PostBloom;
    m_playerPtr = player;
    m_arrowPtr = arrow;
    m_teacherItem = teacher;
    m_inputPtr = &inputEnabled;
    inputEnabled = false;
    if (arrow) arrow->setVisible(false);

    // ① 对话
    QStringList lines = {
        QStringLiteral("老师：很好，看来你已经熟悉贝克思贝斯之剑的用法了！"),
        QStringLiteral("我们换一个空旷的地方吧。")
    };
    m_dialogueSystem->showSequence(lines, [=, &inputEnabled, &waters]() {
        // ② 黑屏 、 清树 、 归位 、 亮起
        fadeToBlackAndBack(player, teacher, waters, [=, &inputEnabled, &waters]() {
            // ③ 恩特之盾教学对话
            m_phase = Phase::ShieldTutorial;
            QStringList lines2 = {
                QStringLiteral("老师：接下来试试恩特之盾吧。"),
                QStringLiteral("恩特之盾可以用来规避敌人的弹幕攻击，"),
                QStringLiteral("面朝弹幕按回车键即可。"),
                QStringLiteral("你问我是怎么知道回车键的？"),
                QStringLiteral("打破第四面墙是什么很难的操作吗？"),
                QStringLiteral("时间紧迫，快去站到那里，我们开始对练。")
            };
            m_dialogueSystem->showSequence(lines2, [=, &inputEnabled]() {
                // ④ 自动走到 (12,4)
                m_phase = Phase::WalkToPos;
                autoWalkPlayer(player, QPoint(12, 4), [=, &inputEnabled]() {
                    // ⑤ 弹幕前对话
                    m_phase = Phase::BulletIntro;
                    QStringList lines3 = {
                        QStringLiteral("老师：试着规避我朝你发射的弹幕吧！"),
                        QStringLiteral("我：老师为什么会发射弹幕！我真的不是在做梦吗！"),
                        QStringLiteral("没时间细想了，老师攻过来了！")
                    };
                    m_dialogueSystem->showSequence(lines3, [=, &inputEnabled]() {
                        // ⑥ 弹幕开始
                        startBulletHell(player, inputEnabled);
                    });
                });
            });
        });
    });
}

//  黑屏转场
void Level7::fadeToBlackAndBack(QGraphicsTextItem *player, QGraphicsTextItem *teacher,
                                 QList<QGraphicsTextItem*> &waters,
                                 std::function<void()> onDone) {
    auto *blackout = m_scene->addRect(m_scene->sceneRect(),
                                       QPen(Qt::NoPen), QBrush(Qt::black));
    blackout->setZValue(200);
    blackout->setOpacity(0.0);

    Anim::variantAnim(this, 0.0, 1.0, 600, QEasingCurve::InCubic,
        [blackout](qreal v) { blackout->setOpacity(v); },
        [=, &waters]() {
        clearAllTreeItems(waters);
        if (player) {
            player->setPos(Grid::toPixel(1, 4));
            player->setOpacity(1.0);
        }
        if (teacher) {
            teacher->setPos(Grid::toPixel(2, 4));
            teacher->setOpacity(1.0);
        }
        QTimer::singleShot(1500, this, [=]() {
            Anim::variantAnim(this, 1.0, 0.0, 600, QEasingCurve::OutCubic,
                [blackout](qreal v) { blackout->setOpacity(v); },
                [=]() {
                    m_scene->removeItem(blackout);
                    delete blackout;
                    if (onDone) onDone();
                });
        });
    });
}

void Level7::clearAllTreeItems(QList<QGraphicsTextItem*> &waters) {
    // 先从 waters 列表中移除将要被删除的 item，防止野指针
    for (auto &sw : m_treeAnims) {
        if (sw.item) waters.removeAll(sw.item);
    }
    // 停止动画并删除所有树字、花字 item
    for (auto &sw : m_treeAnims) {
        if (sw.anim) { sw.anim->stop(); delete sw.anim; sw.anim = nullptr; }
        if (sw.item) {
            if (sw.item->scene()) sw.item->scene()->removeItem(sw.item);
            delete sw.item;
        }
    }
    m_treeAnims.clear();
}

//  自动行走
void Level7::autoWalkPlayer(QGraphicsTextItem *player, QPoint target,
                             std::function<void()> onDone) {
    if (!player) { if (onDone) onDone(); return; }

    QPoint start = Grid::toGrid(player->pos());
    // 简单直线：先水平再垂直
    struct Step { QPoint grid; };
    QList<Step> steps;

    int dx = (target.x() > start.x()) ? 1 : (target.x() < start.x() ? -1 : 0);
    int dy = (target.y() > start.y()) ? 1 : (target.y() < start.y() ? -1 : 0);

    QPoint cur = start;
    while (cur.x() != target.x()) { cur.setX(cur.x() + dx); steps.append({cur}); }
    while (cur.y() != target.y()) { cur.setY(cur.y() + dy); steps.append({cur}); }

    if (steps.isEmpty()) { if (onDone) onDone(); return; }

    auto *timer = new QTimer(this);
    auto *stepIdx = new int(0);
    auto *stepsCopy = new QList<Step>(steps);

    connect(timer, &QTimer::timeout, this, [=]() mutable {
        if (*stepIdx >= stepsCopy->size()) {
            timer->stop(); timer->deleteLater();
            delete stepIdx; delete stepsCopy;
            if (onDone) onDone();
            return;
        }
        QPointF targetPos = Grid::toPixel(stepsCopy->at(*stepIdx).grid.x(),
                                           stepsCopy->at(*stepIdx).grid.y());
        Anim::slide(this, player, targetPos, 120);
        (*stepIdx)++;
    });
    timer->start(150);
}

//  弹幕系统
void Level7::startBulletHell(QGraphicsTextItem *player, bool &inputEnabled) {
    Q_UNUSED(inputEnabled);
    m_phase = Phase::BulletHell;
    m_bulletX = 3.0;
    m_bulletPhase = 0;
    m_bulletDodged = false;
    m_waitingForEnter = false;
    m_bullets.clear();

    QFont bulletFont(QStringLiteral("黑体"), 24, QFont::Bold);

    for (int gy : {3, 4, 5}) {
        auto *b = m_scene->addText(QStringLiteral("弹"), bulletFont);
        b->setDefaultTextColor(Qt::white);
        b->setPos(Grid::toPixel(3, gy));
        b->setOpacity(0.0);
        b->setZValue(30);

        // 发光特效
        auto *glow = new QGraphicsDropShadowEffect;
        glow->setBlurRadius(22);
        glow->setColor(QColor(255, 180, 40, 220));
        glow->setOffset(0, 0);
        b->setGraphicsEffect(glow);

        m_bullets.append(b);
    }

    if (m_bulletTimer) { m_bulletTimer->stop(); delete m_bulletTimer; }
    m_bulletTimer = new QTimer(this);
    connect(m_bulletTimer, &QTimer::timeout, this, [this, player]() {
        tickBullets(player);
    });

    // Phase 0: 渐显（0.5s），每 50ms 更新一次
    m_bulletTimer->start(50);

    // 弹幕阶段结束后 、 闪避训练
    connect(this, &Level7::bulletHellComplete, this,
            &Level7::onBulletHellFinished, Qt::SingleShotConnection);
}

void Level7::tickBullets(QGraphicsTextItem *player) {
    Q_UNUSED(player);

    if (m_bulletPhase == 0) {
        // 渐显阶段：线性增加 opacity
        m_fadeTick++;
        qreal alpha = m_fadeTick / 10.0;  // 10 ticks × 50ms = 500ms
        if (alpha > 1.0) alpha = 1.0;
        for (auto *b : m_bullets)
            if (b) b->setOpacity(alpha);
        if (alpha >= 1.0) {
            m_fadeTick = 0;
            m_bulletPhase = 1;  // 进入移动阶段
        }
        return;
    }

    // 移动 / 慢放 / 冻结 阶段
    if (m_bulletPhase >= 1 && m_bulletPhase <= 3) {
        // 移动步长：统一 0.167 格/帧（50ms时 = 0.3s/格）
        // 慢放通过增大 timer 间隔实现，不改变步长
        const qreal step = 0.167;

        if (m_bulletPhase != 3)
            m_bulletX += step;

        for (auto *b : m_bullets) {
            if (!b) continue;
            int gy = Grid::toGrid(b->pos()).y();
            b->setPos(m_bulletX * Grid::kSize, gy * Grid::kSize);
        }

        // 到达 x=10 、 慢放
        if (m_bulletPhase == 1 && m_bulletX >= 10.0) {
            m_bulletPhase = 2;
            m_bulletX = 10.0;
            m_bulletTimer->setInterval(200);  // 4x 慢放
        }

        // 到达 x=11 、 冻结
        if (m_bulletPhase == 2 && m_bulletX >= 11.0) {
            m_bulletPhase = 3;
            m_bulletTimer->stop();

            // 闪烁提示
            m_waitingForEnter = true;
            QFont hintFont(QStringLiteral("黑体"), 12);
            m_enterHint = m_scene->addText(QStringLiteral("按Enter键规避！"), hintFont);
            m_enterHint->setDefaultTextColor(Qt::white);
            m_enterHint->setPos(20, 560);
            m_enterHint->setZValue(300);

            m_enterHintTimer = new QTimer(this);
            connect(m_enterHintTimer, &QTimer::timeout, this, [this, t = 0]() mutable {
                if (!m_enterHint) return;
                t++;
                m_enterHint->setVisible(t % 6 < 3);  // 快速闪烁
            });
            m_enterHintTimer->start(80);
        }
        return;
    }

    // Phase 4: 规避后继续移动
    if (m_bulletPhase == 4) {
        const qreal step = 0.167;
        m_bulletX += step;

        for (int i = 0; i < m_bullets.size(); ++i) {
            auto *b = m_bullets[i];
            if (!b) continue;
            if (!b->isVisible()) continue;  // y=4 躲开后不再更新

            int gy = Grid::toGrid(b->pos()).y();
            qreal bx = (gy == 4) ? (13.0 + (m_bulletX - 11.0)) : m_bulletX;
            b->setPos(bx * Grid::kSize, gy * Grid::kSize);

            // 移出屏幕右侧
            if (bx >= 19.0) {
                m_bullets[i] = nullptr;      // ★ 立即清除指针，防止后续 tick 访问野指针
                Anim::fade(this, b, 0.0, 300, [this, b]() {
                    m_scene->removeItem(b); delete b;
                });
            }
        }

        // 全部消失 、 完成
        bool allGone = true;
        for (auto *b : m_bullets) {
            if (b && b->isVisible()) { allGone = false; break; }
        }
        if (allGone) {
            if (m_bulletTimer) { m_bulletTimer->stop(); delete m_bulletTimer; m_bulletTimer = nullptr; }
            m_bullets.clear();
            m_bulletPhase = 5;
            m_phase = Phase::Done;
            emit bulletHellComplete();
        }
    }
}

//  弹幕结束 、 闪避训练
void Level7::onBulletHellFinished() {
    if (!m_playerPtr || !m_dialogueSystem || !m_inputPtr) return;
    m_phase = Phase::DodgeIntro;
    *m_inputPtr = false;

    QStringList lines = {
        QStringLiteral("老师：接下来我会增加难度，保持注意力集中！"),
        QStringLiteral("我：不会吧，还来！")
    };
    m_dialogueSystem->showSequence(lines, [this]() {
        if (!m_playerPtr || !m_teacherItem || !m_arrowPtr || !m_inputPtr) return;
        startDodgeTraining(m_playerPtr, m_teacherItem, m_arrowPtr, *m_inputPtr);
    });
}

//  闪避训练：老师 AI 随机移动 + 发射弹幕 + 倒计时 + HP
void Level7::startDodgeTraining(QGraphicsTextItem *player, QGraphicsTextItem *teacher,
                                 QGraphicsTextItem *arrow, bool &inputEnabled) {
    m_phase = Phase::DodgeTraining;
    inputEnabled = true;
    if (arrow) arrow->setVisible(true);

    m_hp = 3;
    m_countdownSec = 20;
    m_fireCooldown = 0;
    clearFlyingBullets();

    // HUD
    updateHud();

    // 老师移动定时器（~180ms/步）
    if (m_teacherTimer) { m_teacherTimer->stop(); delete m_teacherTimer; }
    m_teacherTimer = new QTimer(this);
    connect(m_teacherTimer, &QTimer::timeout, this, [this, player]() {
        tickTeacher(player);
    });
    m_teacherTimer->start(180);

    // 弹幕物理定时器（~50ms）
    if (m_dodgeTimer) { m_dodgeTimer->stop(); delete m_dodgeTimer; }
    m_dodgeTimer = new QTimer(this);
    connect(m_dodgeTimer, &QTimer::timeout, this, [this, player, arrow]() {
        tickDodge(player, arrow);
    });
    m_dodgeTimer->start(50);

    // 倒计时定时器
    if (m_countdownTimer) { m_countdownTimer->stop(); delete m_countdownTimer; }
    m_countdownTimer = new QTimer(this);
    connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
        m_countdownSec--;
        updateHud();
        if (m_countdownSec <= 0 || m_hp <= 0) {
            endDodgeTraining(m_hp > 0);
        }
    });
    m_countdownTimer->start(1000);
}

void Level7::tickDodge(QGraphicsTextItem *player, QGraphicsTextItem *arrow) {
    Q_UNUSED(arrow);
    if (!player || m_phase != Phase::DodgeTraining) return;
    const qreal speed = 0.333; // 0.15s/格 、 step = 50/150 = 0.333格/帧
    for (int i = m_flyingBullets.size() - 1; i >= 0; --i) {
        auto &fb = m_flyingBullets[i];
        if (!fb.item) { m_flyingBullets.removeAt(i); continue; }
        fb.x += fb.dx * speed * Grid::kSize;
        fb.y += fb.dy * speed * Grid::kSize;
        // 飞出屏幕
        if (fb.x < -40 || fb.x > 840 || fb.y < -40 || fb.y > 640) {
            m_scene->removeItem(fb.item); delete fb.item;
            m_flyingBullets.removeAt(i);
        } else {
            fb.item->setPos(fb.x, fb.y);
        }
    }
    checkDodgeCollision(player);

    // 发射冷却
    if (m_fireCooldown > 0) m_fireCooldown--;
    if (m_fireCooldown <= 0) {
        fireBulletAtPlayer(player);
        m_fireCooldown = 8 + (std::rand() % 20); // 0.4~1.4s 随机间隔
    }
}

void Level7::tickTeacher(QGraphicsTextItem *player) {
    if (!player || !m_teacherItem || m_phase != Phase::DodgeTraining) return;
    QPoint tp = Grid::toGrid(m_teacherItem->pos());
    QPoint pp = Grid::toGrid(player->pos());

    int dx = 0, dy = 0;
    if (qAbs(tp.x() - pp.x()) > 7 || qAbs(tp.y() - pp.y()) > 7) {
        // 向玩家靠近
        dx = (pp.x() > tp.x()) ? 1 : (pp.x() < tp.x() ? -1 : 0);
        dy = (pp.y() > tp.y()) ? 1 : (pp.y() < tp.y() ? -1 : 0);
    } else {
        // 随机移动
        int dir = std::rand() % 4;
        dx = (dir == 0) ? 1 : (dir == 1 ? -1 : 0);
        dy = (dir == 2) ? 1 : (dir == 3 ? -1 : 0);
    }
    if (dx != 0 && dy != 0) { // 禁止斜向
        if (std::rand() % 2) dx = 0; else dy = 0;
    }

    QPoint np = tp + QPoint(dx, dy);
    if (np.x() >= 0 && np.x() <= 19 && np.y() >= 0 && np.y() <= 14) {
        m_teacherItem->setPos(Grid::toPixel(np.x(), np.y()));
    }
}

void Level7::fireBulletAtPlayer(QGraphicsTextItem *player) {
    if (!player || !m_teacherItem || !m_scene) return;

    QPointF tp = m_teacherItem->pos();
    QPointF pp = player->pos();
    qreal diffX = pp.x() - tp.x();
    qreal diffY = pp.y() - tp.y();

    // 量化到 4 方向
    int dx = 0, dy = 0;
    if (qAbs(diffX) >= qAbs(diffY))
        dx = (diffX > 0) ? 1 : -1;
    else
        dy = (diffY > 0) ? 1 : -1;

    QFont f(QStringLiteral("黑体"), 18);
    auto *b = m_scene->addText(QStringLiteral("弹"), f);
    b->setDefaultTextColor(QColor(255, 200, 60));
    b->setPos(tp);
    b->setZValue(30);

    auto *glow = new QGraphicsDropShadowEffect;
    glow->setBlurRadius(18);
    glow->setColor(QColor(255, 180, 40, 200));
    glow->setOffset(0, 0);
    b->setGraphicsEffect(glow);

    m_flyingBullets.append({b, static_cast<qreal>(tp.x()), static_cast<qreal>(tp.y()), dx, dy});
}

void Level7::checkDodgeCollision(QGraphicsTextItem *player) {
    if (!player) return;
    QPoint pg = Grid::toGrid(player->pos());
    bool hit = false;

    for (int i = m_flyingBullets.size() - 1; i >= 0; --i) {
        auto &fb = m_flyingBullets[i];
        if (!fb.item) { m_flyingBullets.removeAt(i); continue; }

        QPoint bg = Grid::toGrid(QPointF(fb.x, fb.y));
        if (bg == pg) {
            // 击中玩家
            m_scene->removeItem(fb.item);
            delete fb.item;
            m_flyingBullets.removeAt(i);
            hit = true;
        }
    }

    if (hit) {
        m_hp--;
        updateHud();
        // 受伤闪烁
        if (player) {
            player->setOpacity(0.3);
            QTimer::singleShot(150, this, [player]() {
                if (player) player->setOpacity(1.0);
            });
        }
        if (m_hp <= 0) {
            endDodgeTraining(false);
        }
    }
}

void Level7::updateHud() {
    if (!m_scene) return;
    // HP
    QString hpText = QStringLiteral("我 x %1").arg(m_hp);
    if (!m_hpDisplay) {
        m_hpDisplay = m_scene->addText(hpText, QFont(QStringLiteral("黑体"), 18));
        m_hpDisplay->setDefaultTextColor(QColor(255, 100, 100));
        m_hpDisplay->setZValue(300);
    } else {
        m_hpDisplay->setPlainText(hpText);
    }
    m_hpDisplay->setPos(Grid::toPixel(14, 13));

    // 倒计时
    QString timeText = QStringLiteral("倒计时 %1s").arg(m_countdownSec);
    if (!m_timerDisplay) {
        m_timerDisplay = m_scene->addText(timeText, QFont(QStringLiteral("黑体"), 18));
        m_timerDisplay->setDefaultTextColor(Qt::white);
        m_timerDisplay->setZValue(300);
    } else {
        m_timerDisplay->setPlainText(timeText);
    }
    m_timerDisplay->setPos(Grid::toPixel(14, 14));
}

void Level7::endDodgeTraining(bool success) {
    m_phase = Phase::Done;
    if (m_teacherTimer) { m_teacherTimer->stop(); }
    if (m_dodgeTimer) { m_dodgeTimer->stop(); }
    if (m_countdownTimer) { m_countdownTimer->stop(); }
    clearFlyingBullets();

    if (m_hpDisplay) { m_scene->removeItem(m_hpDisplay); delete m_hpDisplay; m_hpDisplay = nullptr; }
    if (m_timerDisplay) { m_scene->removeItem(m_timerDisplay); delete m_timerDisplay; m_timerDisplay = nullptr; }

    if (success && m_dialogueSystem) {
        *m_inputPtr = false;
        if (m_arrowPtr) m_arrowPtr->setVisible(false);
        playEndingSequence(0);
    } else if (!success && m_dialogueSystem) {
        m_dialogueSystem->display(QStringLiteral("生命值耗尽，重新开始..."), 3, 12, 16, [this]() {
            if (m_playerPtr && m_teacherItem && m_arrowPtr && m_inputPtr) {
                // 短暂延迟防 Enter 键盘重复穿透
                m_phase = Phase::Done;
                QTimer::singleShot(300, this, [this]() {
                    AchievementManager::instance()->unlock(AchievementId::ReBuQi);
                    startDodgeTraining(m_playerPtr, m_teacherItem,
                                       m_arrowPtr, *m_inputPtr);
                });
            }
        });
    }
}

void Level7::clearFlyingBullets() {
    for (auto &fb : m_flyingBullets) {
        if (fb.item && fb.item->scene()) {
            fb.item->scene()->removeItem(fb.item);
            delete fb.item;
        }
    }
    m_flyingBullets.clear();
}

//  结局序列
void Level7::playEndingSequence(int idx) {
    if (!m_dialogueSystem || !m_playerPtr || !m_teacherItem) return;

    const QStringList lines = {
        QStringLiteral("老师：太好了，你对这两件神器的运用简直得心应手！"),
        QStringLiteral("————就像天天和它们打交道一样！"),
        QStringLiteral("（轰隆!）"),
        QStringLiteral("老师：不好了，恶龙似乎又在破坏校园！"),
        QStringLiteral("听声音......是在院机房1235，快去阻止它！"),
        QStringLiteral("我：恶龙为什么会出现在机房啊！"),
        QStringLiteral("还有老师您是怎么听出来具体房间的啊！"),
        QStringLiteral("槽点太多，无力挨个吐槽了......"),
        QStringLiteral("不管了，先走吧。")
    };

    if (idx >= lines.size()) {
        animateExitAndComplete();
        return;
    }

    // 轰隆 、 屏幕震动
    if (idx == 2) shakeScreen();

    m_dialogueSystem->display(lines[idx], 2, 12, 18, [this, idx]() {
        playEndingSequence(idx + 1);
    });
}

void Level7::animateExitAndComplete() {
    QPointF corner = Grid::toPixel(18, 13);
    QPointF teacherCorner = corner + QPointF(Grid::kSize, 0);

    auto *moveGroup = new QParallelAnimationGroup;
    moveGroup->addAnimation(Anim::prop(this, m_playerPtr, "pos", 1200, corner, QEasingCurve::InOutCubic));
    moveGroup->addAnimation(Anim::prop(this, m_teacherItem, "pos", 1200, teacherCorner, QEasingCurve::InOutCubic));

    auto *fadeGroup = new QParallelAnimationGroup;
    fadeGroup->addAnimation(Anim::prop(this, m_playerPtr, "opacity", 800, 0.0));
    fadeGroup->addAnimation(Anim::prop(this, m_teacherItem, "opacity", 800, 0.0));

    auto *seq = new QSequentialAnimationGroup;
    seq->addAnimation(moveGroup);
    seq->addAnimation(fadeGroup);
    connect(seq, &QSequentialAnimationGroup::finished, this, [this]() {
        emit trainingComplete();
    });
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

void Level7::shakeScreen() {
    QGraphicsView *view = m_view;
    if (!view) return;

    QTransform saved = view->transform();

    Anim::variantAnim(this, 0, 20, 600, QEasingCurve::Linear,
        [view, saved](qreal t) {
            qreal decay = (t < 14) ? 1.0 : 1.0 - (t - 14) / 6.0;
            if (decay < 0.0) decay = 0.0;
            qreal sx = ((std::rand() % 25) - 12) * decay;
            qreal sy = ((std::rand() % 21) - 10) * decay;
            view->setTransform(QTransform(saved).translate(sx, sy));
        },
        [view, saved]() { view->setTransform(saved); });
}

//  清理
void Level7::cleanup() {
    for (auto &sw : m_treeAnims) {
        if (sw.anim) { sw.anim->stop(); delete sw.anim; }
    }
    m_treeAnims.clear();

    if (m_bulletTimer) { m_bulletTimer->stop(); delete m_bulletTimer; m_bulletTimer = nullptr; }
    for (auto *b : m_bullets) {
        if (b && b->scene()) { b->scene()->removeItem(b); delete b; }
    }
    m_bullets.clear();

    if (m_enterHintTimer) { m_enterHintTimer->stop(); delete m_enterHintTimer; m_enterHintTimer = nullptr; }
    if (m_enterHint) { if (m_enterHint->scene()) m_enterHint->scene()->removeItem(m_enterHint); delete m_enterHint; m_enterHint = nullptr; }

    if (m_teacherTimer) { m_teacherTimer->stop(); delete m_teacherTimer; m_teacherTimer = nullptr; }
    if (m_dodgeTimer) { m_dodgeTimer->stop(); delete m_dodgeTimer; m_dodgeTimer = nullptr; }
    if (m_countdownTimer) { m_countdownTimer->stop(); delete m_countdownTimer; m_countdownTimer = nullptr; }
    clearFlyingBullets();
    if (m_hpDisplay) { if (m_hpDisplay->scene()) m_hpDisplay->scene()->removeItem(m_hpDisplay); delete m_hpDisplay; m_hpDisplay = nullptr; }
    if (m_timerDisplay) { if (m_timerDisplay->scene()) m_timerDisplay->scene()->removeItem(m_timerDisplay); delete m_timerDisplay; m_timerDisplay = nullptr; }

    m_bloomedCount = 0;
    for (int i = 0; i < 3; ++i) m_bloomedFlags[i] = false;
    m_phase = Phase::Entry;
    m_waitingForEnter = false;
    m_bulletDodged = false;
    m_bulletX = 3.0;
    m_bulletPhase = 0;
    m_fadeTick = 0;
    m_hp = 3;
    m_countdownSec = 20;
    m_playerPtr = nullptr;
    m_arrowPtr = nullptr;
    m_teacherItem = nullptr;
    m_inputPtr = nullptr;

    LevelBase::cleanup();
}
