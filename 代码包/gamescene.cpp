#include "gamescene.h"
#include "animutils.h"
#include "grid.h"
#include "achievement.h"
#include "dialoguesystem.h"
#include "pausemenu.h"
#include "level1.h"
#include "level2.h"
#include "level3road.h"
#include "level4classroom.h"
#include "fishingmechanic.h"
#include "platformermechanic.h"
#include "level7.h"
#include "level8.h"
#include "deleteeffect.h"
#include "interactionsystem.h"
#include "deatheffect.h"
#include "level9.h"
#include "level10.h"
#include "level11.h"
#include "level12.h"

#include <QFont>
#include <QFontMetrics>
#include <QBrush>
#include <QPen>
#include <QTimer>
#include <QGraphicsRectItem>
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QPolygonF>
#include <QQueue>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QVariantAnimation>
#include <QKeyEvent>
#include <QGraphicsView>
#include <algorithm>
#include <QPointer>
#include <QDebug>
#include <cstdlib>
#include <cmath>

// 构造函数
GameScene::GameScene(QObject *parent) : QGraphicsScene(parent) {
    setSceneRect(0, 0, 800, 600);
    setBackgroundBrush(QBrush(Qt::black));

    // 创建子系统
    m_dialogueSystem = new DialogueSystem(this);
    m_dialogueSystem->setScene(this);

    m_pauseMenu = new PauseMenu(this);
    m_pauseMenu->setScene(this);

    m_level1 = new Level1(this);
    m_level1->setScene(this);
    m_level1->setDialogueSystem(m_dialogueSystem);

    m_level2 = new Level2(this);
    m_level2->setScene(this);
    m_level2->setDialogueSystem(m_dialogueSystem);

    m_level3Road = new Level3Road(this);
    m_level3Road->setScene(this);
    m_level3Road->setDialogueSystem(m_dialogueSystem);

    m_level4Classroom = new Level4Classroom(this);
    m_level4Classroom->setScene(this);
    m_level4Classroom->setDialogueSystem(m_dialogueSystem);

    m_fishingMechanic = new FishingMechanic(this);
    m_fishingMechanic->setScene(this);
    m_fishingMechanic->setDialogueSystem(m_dialogueSystem);

    m_platformerMechanic = new PlatformerMechanic(this);
    m_platformerMechanic->setScene(this);
    m_platformerMechanic->setDialogueSystem(m_dialogueSystem);

    m_deleteEffect = new DeleteEffect(this);
    m_deleteEffect->setScene(this);

    m_level7 = new Level7(this);
    m_level7->setScene(this);
    m_level7->setDialogueSystem(m_dialogueSystem);
    m_level7->setDeleteEffect(m_deleteEffect);

    m_deathEffect = new DeathEffect(this);
    m_deathEffect->setScene(this);
    m_deathEffect->setDialogueSystem(m_dialogueSystem);

    m_interactionSystem = new InteractionSystem(this);
    m_interactionSystem->setScene(this);
    m_interactionSystem->setDialogueSystem(m_dialogueSystem);
    // 成就 #8 别想太多：F 键与"纸"交互
    connect(m_interactionSystem, &InteractionSystem::interactionStarted, this,
            [](QPoint pos) {
        if (pos == QPoint(3, 8))
            AchievementManager::instance()->unlock(AchievementId::BieXiangTaiDuo);
    });

    m_level8 = new Level8(this);
    m_level8->setScene(this);
    m_level8->setDialogueSystem(m_dialogueSystem);
    m_level8->setInteractionSystem(m_interactionSystem);
    connect(m_level8, &Level8::stonesBroken, this, [this]() {
        m_inputEnabled = false;
        if (m_arrow) m_arrow->setVisible(true);
        startAutoWalk(10, 0);  // 走到 y=0 的门前
    }, Qt::SingleShotConnection);
    connect(m_level8, &Level8::level8Complete, this, [this]() {
        fadeTransition(9);
    }, Qt::SingleShotConnection);

    m_level9 = new Level9(this);
    m_level9->setScene(this);
    m_level9->setDialogueSystem(m_dialogueSystem);
    m_level9->setDeleteEffect(m_deleteEffect);
    m_level9->setDeathEffect(m_deathEffect);
    connect(m_level9, &Level9::level9Complete, this, [this]() {
        fadeTransition(10);
    }, Qt::SingleShotConnection);

    m_level10 = new Level10(this);
    m_level10->setScene(this);
    m_level10->setDialogueSystem(m_dialogueSystem);
    m_level10->setDeleteEffect(m_deleteEffect);
    m_level10->setDeathEffect(m_deathEffect);
    connect(m_level10, &Level10::level10Complete, this, [this]() {
        AchievementManager::instance()->unlock(AchievementId::JieShuLe);
        fadeTransition(11);
    }, Qt::SingleShotConnection);

    m_level11 = new Level11(this);
    m_level11->setScene(this);
    m_level11->setDialogueSystem(m_dialogueSystem);
    connect(m_level11, &Level11::level11Complete, this, [this]() {
        AchievementManager::instance()->unlock(AchievementId::GuiMeng);
        fadeTransition(12);
    }, Qt::SingleShotConnection);

    m_level12 = new Level12(this);
    m_level12->setScene(this);
    m_level12->setDialogueSystem(m_dialogueSystem);
    connect(m_level12, &Level12::level12Complete, this, [this]() {
        // Level12 内部在 phase6 调用了 m_scene->clear()，
        // m_player / m_arrow 等已变成野指针，必须置空防止后续访问崩溃。
        m_player = nullptr;
        m_arrow = nullptr;
        m_pushables.clear();
        m_waters.clear();
        m_specialItems.clear();
        m_traps.clear();
        AchievementManager::instance()->unlock(AchievementId::YiZhouMu);
        SaveData d;
        d.level = 12;
        SaveManager::save("slot12", d);
        emit gameFinished();
    }, Qt::SingleShotConnection);

    // PauseMenu 信号连接
    connect(m_pauseMenu, &PauseMenu::resumeGame, this, [this]() {
        togglePause();  // 即关闭暂停
    });
    connect(m_pauseMenu, &PauseMenu::resetLevelRequested, this, [this]() {
        resetLevel();   // 箭头可见性已由 loadLevel 按关卡正确设置
        m_paused = false;
        m_inputEnabled = true;
        // 平台跳跃由 loadLevel 重新初始化，不需要单独重启
    });
    connect(m_pauseMenu, &PauseMenu::returnToMenuRequested, this, [this]() {
        m_paused = false;
        m_inputEnabled = true;
        qDebug() << "[GameScene] emit levelComplete() from PauseMenu::returnToMenuRequested";
        emit levelComplete();
    });
    connect(m_pauseMenu, &PauseMenu::loadLevelRequested, this, [this](int lvl) {
        m_paused = false;
        loadLevel(lvl);
    });

    // Platformer 支线：碰到云，掉回正下方平台并触发彩蛋
    connect(m_platformerMechanic, &PlatformerMechanic::touchedCloud, this, [this]() {
        AchievementManager::instance()->unlock(AchievementId::DaDaDa);
        m_dialogueSystem->display("诶？云朵！", 1, 12, 16);
        QTimer::singleShot(2600, this, [this]() {
            if (m_currentLevel == 6 && !m_levelPassed)
                m_dialogueSystem->cleanup();
        });
    });

    // Platformer 胜利信号
    connect(m_platformerMechanic, &PlatformerMechanic::winReached, this, [this]() {
        m_dialogueSystem->display("拿到了恩特之盾！", 1, 12, 18, [this]() {
            QTimer::singleShot(1500, this, [this]() {
                m_dialogueSystem->cleanup();
                fadeTransition(m_currentLevel + 1);
            });
        });
    });

    // Fishing 钓鱼完成信号
    connect(m_fishingMechanic, &FishingMechanic::catchComplete, this, [this](bool isTarget) {
        if (isTarget)
            fadeTransition(m_currentLevel + 1);
        else
            m_fishingMechanic->retract();  // wait, retract is private... let me check
    });

}

// 工具函数
QString GameScene::arrowCharForDir(int dx, int dy) const {
    if (dx == 1 && dy == 0)  return QStringLiteral("▶");
    if (dx == -1 && dy == 0) return QStringLiteral("◀");
    if (dx == 0 && dy == -1) return QStringLiteral("▲");
    if (dx == 0 && dy == 1)  return QStringLiteral("▼");
    return QStringLiteral("▶");
}

// 关卡加载
void GameScene::loadLevel(int level) {
    qDebug() << "[GameScene] loadLevel(" << level << ") called";
    cleanupLevelSpecific();

    // 先清理所有引用列表和选择项，避免 clear() 后产生悬空指针 / double-free
    clearChoice();
    m_player = nullptr;
    m_arrow = nullptr;
    m_pushables.clear();
    m_waters.clear();
    m_specialItems.clear();
    m_traps.clear();
    m_trapSet.clear();

    // 然后彻底清除场景中所有残留 item
    clear();
    update();  // 强制刷新，确保上一关残留彻底清除
    m_currentLevel = level;
    m_loadGeneration++;  // 作废旧 load 的所有 QTimer::singleShot 回调
    // 更新到达过的最高关卡进度
    if (level > SaveManager::loadProgress())
        SaveManager::saveProgress(level);
    m_levelPassed = false;

    m_moving = false;
    m_inputEnabled = true;
    m_autoWalkTarget = {-1, -1};
    m_climbWinPos = {-1, -1};
    m_climbStartPos = {2, 13};

    LevelData data;
    if (level == 1)      data = createLevel1();
    else if (level == 2) data = createLevel2();
    else if (level == 3) data = createLevel3();
    else if (level == 4) data = createLevel4();
    else if (level == 5) data = createLevel5();
    else if (level == 6) data = createLevel6();
    else if (level == 7) data = createLevel7();
    else if (level == 8) data = createLevel8();
    else if (level == 9) data = createLevel9();
    else if (level == 10) data = createLevel10();
    else if (level == 11) data = createLevel11();
    else if (level == 12) data = createLevel12();
    else return;

    m_mechanic = data.mechanic;
    m_winCondition = data.winCondition;
    m_winTarget = data.winTarget;
    m_hasWinAnimation = data.hasWinAnimation;
    m_winFrameWords = data.winFrameWords;
    m_comboRules = data.comboRules;
    m_phraseRules = data.phraseRules;
    // 不在此处设置 m_dialogues，由各关卡的叙事启动函数自行设置
    m_climbWinPos = QPoint(data.climbWinX, data.climbWinY);

    for (const auto &p : data.trapPositions)
        m_trapSet.insert({p.x(), p.y()});

    QFont standardFont("黑体", 24);
    QColor white(Qt::white);

    for (const auto &item : data.items) {
        QGraphicsTextItem *ti = addText(item.text, standardFont);
        ti->setDefaultTextColor(white);
        ti->setPos(Grid::toPixel(item.gx, item.gy));
        if (item.customColor.isValid())
            ti->setDefaultTextColor(item.customColor);
        if (item.rotation != 0) {
            ti->setTransformOriginPoint(ti->boundingRect().center());
            ti->setRotation(item.rotation);
        }

        switch (item.type) {
        case LevelItem::Player:
            m_player = ti;
            m_climbStartPos = QPoint(item.gx, item.gy);
            break;
        case LevelItem::Pushable:
            m_pushables.append(ti);
            break;
        case LevelItem::Water:
            m_waters.append(ti);
            break;
        case LevelItem::Special:
            m_specialItems.append(ti);
            if (m_mechanic == LevelData::TowerClimb) ti->setData(1, true);
            break;
        case LevelItem::Trap:
            m_traps.append(ti);
            break;
        case LevelItem::Scenery:
            if (m_mechanic == LevelData::TowerClimb) ti->setData(1, true);
            break;
        }
    }

    // 创建箭头
    m_arrow = addText("▶", standardFont);
    m_arrow->setDefaultTextColor(Qt::white);
    m_arrow->setData(0, true);   // 标记为非平台，爬塔揭示时跳过
    m_dirX = 1; m_dirY = 0;
    m_arrowEnabled = true;
    updateArrow();

    // —— 关卡特定初始化（委托给子系统） ——
    if (m_mechanic == LevelData::Fishing && level != 5) {
        m_arrowEnabled = false;
        if (m_arrow) m_arrow->setVisible(false);
        m_fishingMechanic->setup(m_player, data.fishingItems, data.fishingTargetIndex);
    }

    if (m_mechanic == LevelData::TowerClimb && level != 6) {
        m_arrowEnabled = false;
        if (m_arrow) m_arrow->setVisible(false);
        m_platformerMechanic->start(m_player, m_climbWinPos, m_climbStartPos, m_levelPassed, m_inputEnabled);
    }

    if (level == 3) {
        m_arrowEnabled = false;
        if (m_arrow) m_arrow->setVisible(false);
        m_level3Road->setup(m_pushables, m_waters, m_specialItems);
    }

    if (level == 4) {
        m_arrowEnabled = false;
        if (m_arrow) m_arrow->setVisible(false);
        m_level4Classroom->setPhase(0);
        m_level4Classroom->setupClassroom(m_waters);
        m_player->setOpacity(0.0);
        QTimer::singleShot(400, this, [this]() {
            m_level4Classroom->startEntry(m_player, m_inputEnabled);
            // entryComplete 时创建提示
            connect(m_level4Classroom, &Level4Classroom::entryComplete, this, [this]() {
                createFlashingHint("前往空位就坐吧！", 0, 11, 18);
            }, Qt::SingleShotConnection);
        });
    }

    if (level == 8) {
        if (m_arrow) m_arrow->setVisible(false);
        connect(m_level8, &Level8::entryComplete, this, [this]() {
            createFlashingHint("朝向物体按F键查看信息", 0, 14, 14);
        }, Qt::SingleShotConnection);
        QTimer::singleShot(400, this, [this]() {
            m_level8->startEntry(m_player, m_arrow, m_inputEnabled);
        });
    }

    if (level == 9) {
        m_arrowEnabled = false;
        if (m_arrow) m_arrow->setVisible(false);
        m_level9->setItemLists(m_pushables, m_waters);
        if (!views().isEmpty())
            m_level9->setView(qobject_cast<QGraphicsView*>(views().first()));
        QTimer::singleShot(400, this, [this]() {
            m_level9->startEntry(m_player, m_arrow, m_inputEnabled);
        });
    }

    if (level == 10) {
        m_arrowEnabled = true;
        if (m_arrow) m_arrow->setVisible(true);
        QTimer::singleShot(400, this, [this]() {
            m_level10->startEntry(m_player, m_arrow, m_inputEnabled);
        });
    }

    if (level == 11) {
        m_arrowEnabled = false;
        if (m_arrow) m_arrow->setVisible(false);
        m_level11->setItemLists(m_pushables, m_waters);
        QTimer::singleShot(400, this, [this]() {
            m_level11->startEntry(m_player, m_arrow, m_inputEnabled);
        });
    }

    if (level == 12) {
        m_arrowEnabled = false;
        if (m_arrow) m_arrow->setVisible(false);

        QTimer::singleShot(400, this, [this]() {
            m_level12->startEntry(m_player, m_arrow, m_inputEnabled);
        });
    }

    if (level == 7) {
        // 入场期间不显示箭头
        if (m_arrow) m_arrow->setVisible(false);

        // 找"师"字并启动入场序列
        QGraphicsTextItem *teacher = nullptr;
        for (auto *item : items()) {
            if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
                if (ti->toPlainText() == "师"
                    && Grid::toGrid(ti->pos()) == QPoint(-2, 4)) {
                    teacher = ti;
                    break;
                }
            }
        }
        m_level7->setupTrees();
        // 传递 View 指针（用于运镜）
        if (!views().isEmpty())
            m_level7->setView(qobject_cast<QGraphicsView*>(views().first()));
        // 连接开花完成信号 、 转场 、 恩特之盾 、 弹幕
        connect(m_level7, &Level7::allBloomed, this, [this, teacher]() {
            m_level7->startPostBloomSequence(m_player, teacher, m_arrow,
                                              m_waters, m_inputEnabled);
        }, Qt::SingleShotConnection);
        // 闪避训练成功 、 渐隐过渡到第八关
        connect(m_level7, &Level7::trainingComplete, this, [this]() {
            fadeTransition(8);
        }, Qt::SingleShotConnection);
        QTimer::singleShot(400, this, [this, teacher]() {
            m_level7->startEntry(m_player, teacher, m_arrow, m_inputEnabled);
        });
    }

    if (!data.tutorialHintText.isEmpty()) {
        createFlashingHint(data.tutorialHintText, data.hintGx, data.hintGy);
    }

    // 通用布景加载（由 JSON "decor" 数组驱动）
    for (const auto &d : data.decors) {
        auto *t = addText(d.text, standardFont);
        t->setDefaultTextColor(d.color.isValid() ? d.color : white);
        t->setPos(Grid::toPixel(d.gx, d.gy));
        if (d.blocking)
            m_waters.append(t);
        if (d.swaying) {
            auto *sway = Anim::sway(this, t, 4.0, 1500);
            if (sway) m_decorAnims.append(sway);
        }
    }

    // 关卡特定布景（复杂矢量图形等）
    if (level == 2) {
        // 机械钟表（食堂下方）
        {
            QPointF cc = Grid::toPixel(3, 4);
            qreal r = 48.0;
            // 外圈
            m_clockFace = addEllipse(cc.x() - r, cc.y() - r, r * 2, r * 2,
                                     QPen(QColor(180, 160, 120), 2.5), Qt::NoBrush);
            // 内圈
            addEllipse(cc.x() - r + 4, cc.y() - r + 4, (r - 4) * 2, (r - 4) * 2,
                       QPen(QColor(140, 120, 80), 1), Qt::NoBrush);
            // 12个刻度
            for (int i = 0; i < 12; ++i) {
                qreal a = (i * 30.0 - 90.0) * M_PI / 180.0;
                bool isBig = (i % 3 == 0);
                qreal inner = r - (isBig ? 8.0 : 5.0);
                qreal outer = r - 1.5;
                addLine(cc.x() + inner * cos(a), cc.y() + inner * sin(a),
                        cc.x() + outer * cos(a), cc.y() + outer * sin(a),
                        QPen(QColor(200, 180, 140), isBig ? 2.0 : 1.0));
            }
            // 中心圆点
            addEllipse(cc.x() - 2.5, cc.y() - 2.5, 5, 5,
                       QPen(Qt::NoPen), QBrush(QColor(200, 180, 140)));
            // 时针：初始指向七点（Qt坐标系中120° from x轴）
            qreal startAngle = 120.0 * M_PI / 180.0;
            qreal handLen = r - 10.0;
            m_clockHand = addLine(cc.x(), cc.y(),
                                  cc.x() + handLen * cos(startAngle),
                                  cc.y() + handLen * sin(startAngle),
                                  QPen(QColor(255, 220, 160), 3.0));
        }
        // 右上角菜单
        {
            QFont menuFont("黑体", 14);
            QFontMetrics fm14(menuFont);
            QFontMetrics fm24(standardFont);
            QColor menuColor(200, 180, 140);
            // "单"右边缘 = x(18) + "单"在24号字下的宽度
            qreal rightEdge = Grid::toPixel(18, 0).x() + fm24.horizontalAdvance("单");
            auto addMenuItem = [&](const QString &text, int gy) {
                auto *t = addText(text, menuFont);
                t->setDefaultTextColor(menuColor);
                qreal w = fm14.horizontalAdvance(text);
                t->setPos(rightEdge - w, Grid::toPixel(0, gy).y());
            };
            addMenuItem("襄阳牛肉面", 2);
            addMenuItem("炸酱面",   3);
            addMenuItem("热干面",   4);
            addMenuItem("鸡汤面",   5);
        }
    } else if (level == 6) {
        // 由多个小号"云"字拼成一朵云，在天空中缓慢横向飘动
        auto addCloud = [this](qreal startPx, qreal yPx, qreal speedPxPerSec) {
            QFont cloudFont("黑体", 13);
            const QVector<QPoint> shape = {
                {1, 0}, {2, 0}, {3, 0},
                {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}
            };
            const qreal sx = 16.0, sy = 15.0;
            auto *group = new QGraphicsItemGroup();
            for (const QPoint &p : shape) {
                auto *c = new QGraphicsTextItem("云", group);
                c->setFont(cloudFont);
                c->setDefaultTextColor(QColor(200, 200, 215));
                c->setPos(p.x() * sx, p.y() * sy);
                c->setData(0, QStringLiteral("cloud"));
            }
            addItem(group);
            group->setZValue(-1);
            group->setPos(startPx, yPx);

            const qreal cloudW = 4 * sx + 24.0;
            const qreal startLeft = -cloudW;
            const qreal endPx = 800.0 + 24.0;

            auto *anim = new QVariantAnimation(this);
            anim->setEasingCurve(QEasingCurve::Linear);
            anim->setStartValue(startPx);
            anim->setEndValue(endPx);
            anim->setDuration(int((endPx - startPx) / speedPxPerSec * 1000.0));
            connect(anim, &QVariantAnimation::valueChanged, this,
                    [group, yPx](const QVariant &v) {
                        group->setPos(v.toDouble(), yPx);
                    });
            connect(anim, &QVariantAnimation::finished, this,
                    [anim, startLeft, endPx, speedPxPerSec]() {
                        anim->setStartValue(startLeft);
                        anim->setEndValue(endPx);
                        anim->setDuration(
                            int((endPx - startLeft) / speedPxPerSec * 1000.0));
                        anim->start();
                    });
            anim->start();
            m_decorAnims.append(anim);
        };

        addCloud(60,  20,  10.0);
        addCloud(360, 60,  7.0);
        addCloud(620, 35,  13.0);

        // —— 塔身轮廓装饰：纯矢量背景，不会被爬塔逻辑当成平台 ——
        const QColor towerColor(135, 125, 160);
        const QColor baseColor(102, 92, 80);
        const QColor spireColor(214, 188, 122);
        const qreal Z = -0.5;
        auto colX = [](qreal col) { return col * 40.0; };

        QPen bodyPen(towerColor, 1.5);
        addLine(180, 520, 340, 40, bodyPen)->setZValue(Z);
        addLine(620, 520, 460, 40, bodyPen)->setZValue(Z);

        struct Tier { int gy; int colL; int colR; };
        const QVector<Tier> tiers = {
            {13, 4, 15}, {11, 4, 15}, {9, 8, 11}, {7, 6, 13},
            {5, 8, 11}, {3, 7, 12}, {1, 8, 11}
        };
        QPen eavePen(towerColor, 2.0);
        for (const Tier &t : tiers) {
            qreal y  = t.gy * 40.0;
            qreal xL = colX(t.colL), xR = colX(t.colR + 1);
            addLine(xL - 8, y, xR + 8, y, eavePen)->setZValue(Z);
            addLine(xL - 8, y, xL - 16, y - 9, eavePen)->setZValue(Z);
            addLine(xR + 8, y, xR + 16, y - 9, eavePen)->setZValue(Z);
        }

        addLine(150, 566, 650, 566, QPen(baseColor, 4.0))->setZValue(Z);
        addLine(120, 582, 680, 582, QPen(baseColor, 7.0))->setZValue(Z);

        const qreal cx = 400.0;
        QPen spirePen(spireColor, 2.5);
        QBrush spireBrush(spireColor);
        addLine(cx, 40, cx, 16, spirePen)->setZValue(Z);
        addEllipse(cx - 4, 9, 8, 8, spirePen, spireBrush)->setZValue(Z);
        QPolygonF tip;
        tip << QPointF(cx, 0) << QPointF(cx - 5, 11) << QPointF(cx + 5, 11);
        addPolygon(tip, spirePen, spireBrush)->setZValue(Z);
    }

    // —— 关卡开场对话（打字机效果） ——
    if (level == 1) {
        m_level1->startEntry(m_player, m_arrow, m_inputEnabled, m_arrowEnabled,
            [this](const QString &text, int gx, int gy,
                   const QSet<int> &pushable, std::function<void()> onDone) {
                startTextReveal(text, gx, gy, pushable, std::move(onDone));
            });
    } else if (level == 2) {
        m_level2->startEntry(m_player, m_arrow, m_inputEnabled, m_arrowEnabled,
            [this](const QString &text, int gx, int gy,
                   const QSet<int> &pushable, std::function<void()> onDone) {
                startTextReveal(text, gx, gy, pushable, std::move(onDone));
            });
    } else if (level == 5) {
        m_fishingMechanic->startLevel5Entry(m_player, m_arrow, m_inputEnabled,
            m_arrowEnabled, data.fishingItems, data.fishingTargetIndex);
    } else if (level == 6) {
        m_platformerMechanic->startLevel6Entry(m_player, m_arrow, m_inputEnabled,
            m_arrowEnabled, m_climbWinPos, m_climbStartPos, m_levelPassed);
    }

    checkLevelComplete();

    qDebug() << "[GameScene] loadLevel(" << level << ") done. mechanic=" << (int)m_mechanic
             << "player=" << (m_player != nullptr) << "arrow=" << (m_arrow != nullptr)
             << "pushables=" << m_pushables.size() << "waters=" << m_waters.size();

    // 通知 MainWindow 关卡已切换，触发音乐切换
    emit levelChanged(m_currentLevel);
}

// 移动逻辑
void GameScene::movePlayer(int dx, int dy) {
    if (!m_player || m_moving || !m_inputEnabled) return;
    if (m_currentLevel == 10) return;  // Level 10 uses continuous movement via tick
    if (m_platformerMechanic->isActive()) return;
    if (m_autoWalkTimer && m_autoWalkTimer->isActive()) return;

    hideRevealCursor();

    QPoint currentGrid = Grid::toGrid(m_player->pos());
    QPoint newGrid = currentGrid + QPoint(dx, dy);

    if (isOutOfBounds(newGrid)) return;

    // 关卡3：不可横穿道路（y<=9 禁止）
    if (m_currentLevel == 3 && newGrid.y() <= 9) {
        if (m_level3Road->handleRoadBlock(m_player))
            return;
    }

    // 钓鱼：左右自由移动，y 固定在 0
    if (m_mechanic == LevelData::Fishing) {
        newGrid.setY(0);
        if (isOutOfBounds(newGrid)) return;
        m_moving = true;
        m_dirX = dx; m_dirY = 0;

        QPointF playerTarget = Grid::toPixel(newGrid.x(), 0);

        Anim::slide(this, m_player, playerTarget, 100, [=]() {
            m_moving = false;
        });
        return;
    }

    // 非爬塔模式的水障碍
    if (m_mechanic != LevelData::TowerClimb && isBlockedByWater(newGrid)) return;

    QPointF playerTarget = Grid::toPixel(newGrid.x(), newGrid.y());
    QGraphicsTextItem *hitPushable = pushableAtGrid(newGrid);
    QPointF pushableTarget;

    if (hitPushable) {
        QPoint pushableGrid = Grid::toGrid(hitPushable->pos());
        QPoint pushableNewGrid = pushableGrid + QPoint(dx, dy);

        if (isOutOfBounds(pushableNewGrid) ||
            isBlockedByWater(pushableNewGrid) ||
            isBlockedByPushable(pushableNewGrid, hitPushable))
            return;

        if (m_mechanic == LevelData::TowerClimb && !itemAtGrid(pushableNewGrid))
            return;

        pushableTarget = Grid::toPixel(pushableNewGrid.x(), pushableNewGrid.y());
    }

    m_moving = true;
    m_dirX = dx;
    m_dirY = dy;

    m_arrow->setPlainText(arrowCharForDir(dx, dy));
    QPointF arrowTarget = playerTarget + QPointF(dx * Grid::kSize, dy * Grid::kSize);

    QList<QPropertyAnimation*> anims;
    anims.append(Anim::prop(this, m_player, "pos", 100, playerTarget));
    anims.append(Anim::prop(this, m_arrow, "pos", 100, arrowTarget));
    if (hitPushable)
        anims.append(Anim::prop(this, hitPushable, "pos", 100, pushableTarget));

    Anim::parallel(this, anims, [=]() {
        m_moving = false;
        if (m_mechanic == LevelData::TextCombine && hitPushable)
            handleTextCombine(hitPushable);

        if (m_mechanic == LevelData::PhraseReplace || m_mechanic == LevelData::PhraseRearrange)
            checkPhraseFormation();

        if (m_currentLevel == 3) {
            // 成就 #3 卡关：将"车"推到 y=9
            if (hitPushable && hitPushable->toPlainText() == QStringLiteral("车")) {
                if (Grid::toGrid(hitPushable->pos()).y() == 9)
                    AchievementManager::instance()->unlock(AchievementId::KaGuan);
            }
            m_level3Road->checkWin(m_player, m_pushables,
                                    m_waters, m_specialItems,
                                    m_levelPassed, m_inputEnabled,
                                    [this]() { fadeTransition(4); });
        }

        if (m_currentLevel == 4) {
            // 绊线检测（独立于叙事进度）
            QPoint pg = Grid::toGrid(m_player->pos());
            m_level4Classroom->checkTripWire(pg, m_player, m_waters, m_inputEnabled);

            if (m_level4Classroom->checkProgressTrigger(m_player, QPoint(13, 8), m_inputEnabled)) {
                destroyHint();
                QTimer::singleShot(300, this, [this]() {
                    m_level4Classroom->doNarrative([this]() {
                        m_level4Classroom->doTeacherWalk(m_waters, m_player, [this]() {
                            fadeTransition(5);
                        });
                    });
                });
            }
        }

        // 爬塔陷阱检测
        if (m_mechanic == LevelData::TowerClimb) {
            QPoint pg = Grid::toGrid(m_player->pos());
            if (m_trapSet.contains({pg.x(), pg.y()})) {
                QTimer::singleShot(50, this, [this]() { triggerGameOver("踩到陷阱了！重新开始..."); });
                return;
            }
            if (m_climbWinPos.x() >= 0 && pg == m_climbWinPos) {
                // winReached 信号由 PlatformerMechanic::tick 发射
            }
        }

        checkLevelComplete();
    });
}

// 碰撞检测
bool GameScene::isBlockedByWater(const QPoint &gridPos) const {
    for (auto *water : m_waters)
        if (Grid::toGrid(water->pos()) == gridPos) return true;
    return false;
}

bool GameScene::isBlockedByPushable(const QPoint &gridPos, QGraphicsTextItem *exclude) const {
    for (auto *p : m_pushables) {
        if (p == exclude) continue;
        if (Grid::toGrid(p->pos()) == gridPos) return true;
    }
    return false;
}

bool GameScene::isOutOfBounds(const QPoint &gridPos) const {
    return gridPos.x() < 0 || gridPos.x() > 19 || gridPos.y() < 0 || gridPos.y() > 14;
}

QGraphicsTextItem *GameScene::pushableAtGrid(const QPoint &gridPos) const {
    for (auto *p : m_pushables)
        if (Grid::toGrid(p->pos()) == gridPos) return p;
    return nullptr;
}

QGraphicsTextItem *GameScene::itemAtGrid(const QPoint &gridPos, const QString &text) const {
    for (auto *item : items()) {
        if (item == m_arrow || item == m_player) continue;
        if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
            if (Grid::toGrid(ti->pos()) == gridPos) {
                if (text.isEmpty() || ti->toPlainText() == text)
                    return ti;
            }
        }
    }
    return nullptr;
}

// 文字合并
void GameScene::handleTextCombine(QGraphicsTextItem *movedItem) {
    if (!movedItem || m_comboRules.isEmpty()) return;

    QPoint pushableGrid = Grid::toGrid(movedItem->pos());
    QString pushableText = movedItem->toPlainText();

    for (const auto &rule : m_comboRules) {
        if (pushableText != rule.pushableText) continue;

        QPoint targetGrid = pushableGrid;
        if (rule.adjacencyDir == 0)      targetGrid += QPoint(1, 0);
        else if (rule.adjacencyDir == 1) targetGrid += QPoint(-1, 0);
        else if (rule.adjacencyDir == 2) targetGrid += QPoint(0, 1);
        else if (rule.adjacencyDir == 3) targetGrid += QPoint(0, -1);

        QGraphicsTextItem *targetItem = itemAtGrid(targetGrid, rule.targetText);
        if (!targetItem) continue;

        m_inputEnabled = false;
        destroyHint();

        Anim::parallel(this, {
            Anim::prop(this, movedItem, "opacity", 300, 0.0),
            Anim::prop(this, targetItem, "opacity", 300, 0.0)
        }, [=]() {
            m_pushables.removeOne(movedItem);
            m_specialItems.removeOne(movedItem);
            removeItem(movedItem); delete movedItem;
            m_specialItems.removeOne(targetItem);
            removeItem(targetItem); delete targetItem;

            QFont f("黑体", 24);
            QGraphicsTextItem *combined = addText(rule.resultText, f);
            combined->setDefaultTextColor(Qt::white);
            combined->setPos(Grid::toPixel(targetGrid.x(), targetGrid.y()));
            combined->setOpacity(1.0);

            Anim::fade(this, combined, 0.0, 600, [=]() {
                removeItem(combined); delete combined;
                if (rule.autoWalkX >= 0)
                    startAutoWalk(rule.autoWalkX, rule.autoWalkY);
                else {
                    m_inputEnabled = true;
                    checkLevelComplete();
                }
            });
        });
        return;
    }
}

// 词组检测
void GameScene::checkPhraseFormation() {
    for (const auto &rule : m_phraseRules) {
        QString current;
        for (const auto &pos : rule.positions) {
            if (m_player && Grid::toGrid(m_player->pos()) == pos)
                current += m_player->toPlainText();
            else {
                QGraphicsTextItem *item = itemAtGrid(pos);
                current += item ? item->toPlainText() : QStringLiteral("?");
            }
        }

        if (!rule.badPhrase.isEmpty() && current == rule.badPhrase) {
            m_inputEnabled = false;
            // 成就检测
            if (current == QStringLiteral("早上九点"))
                AchievementManager::instance()->unlock(AchievementId::KaDian);
            else if (current == QStringLiteral("早上十点"))
                AchievementManager::instance()->unlock(AchievementId::KunShiTang);
            QString msg = (current == "早上十点") ? QStringLiteral("健康的早午饭作息，我认可了")
                                                   : QStringLiteral("糟糕，打烊了...");
            int targetHour = (current == "早上十点") ? 10 : 9;
            animateClockHand(targetHour, [this, msg]() {
                triggerGameOver(msg);
            });
            return;
        }

        if (current == rule.winPhrase) {
            m_inputEnabled = false;
            animateClockHand(8, [this]() {
                onPhraseWin();
            });
            return;
        }
    }
}

void GameScene::onPhraseWin() {
    LevelData d;
    if (m_currentLevel == 2)      d = createLevel2();
    else if (m_currentLevel == 3) d = createLevel3();
    else return;

    if (m_currentLevel == 2) {
        m_inputEnabled = false;
        if (m_arrow) m_arrow->setVisible(false);
        // 第一波：队伍前三分之一消失
        QList<QPoint> wave1;
        for (int x = 15; x <= 17; ++x) wave1.append({x, 9});
        fadeOutItems(wave1, [this]() {
            m_dialogueSystem->display("队伍终于往前挪动了...", 6, 11, 22, [this]() {
                // 第二波：剩余队伍消失
                QList<QPoint> wave2;
                for (int x = 11; x <= 14; ++x) wave2.append({x, 9});
                fadeOutItems(wave2, [this]() {
                    // 菜单"鸡汤面"高亮
                    for (auto *item : items()) {
                        if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
                            if (ti->toPlainText() == "鸡汤面") {
                                auto *flash = new QTimer(this);
                                flash->setObjectName("menuHighlight");
                                int *tick = new int(0);
                                connect(flash, &QTimer::timeout, this, [ti, tick]() {
                                    (*tick)++;
                                    int v = 128 + 127 * std::sin((*tick) * 0.3);
                                    ti->setDefaultTextColor(QColor(255, 220, v));
                                });
                                flash->start(50);
                                break;
                            }
                        }
                    }
                    startAutoWalk(17, 9);
                });
            });
        });
    } else if (m_currentLevel == 3) {
        QGraphicsTextItem *car = nullptr;
        for (auto *sp : m_specialItems) {
            if (sp->toPlainText() == "车") { car = sp; break; }
        }
        if (car) {
            QPointF playerPos = m_player->pos();
            QPointF carTarget(playerPos.x() - Grid::kSize, playerPos.y());

            Anim::slide(this, car, carTarget, 800, [=]() {
                QPointF offScreen(-200, playerPos.y());
                Anim::parallel(this, {
                    Anim::prop(this, m_player, "pos", 1200, offScreen, QEasingCurve::InQuad),
                    Anim::prop(this, car, "pos", 1200, offScreen - QPointF(Grid::kSize, 0), QEasingCurve::InQuad)
                }, [=]() {
                    fadeTransition(m_currentLevel + 1);
                });
            }, QEasingCurve::InOutQuad);
        }
    }
}

// 钟表旋转动画
void GameScene::animateClockHand(int targetHour, std::function<void()> onDone) {
    if (!m_clockHand) { if (onDone) onDone(); return; }

    QPointF cc = m_clockHand->line().p1(); // 钟表中心
    // 读取当前时针角度（atan2空间：从x轴逆时针）
    QLineF curLine = m_clockHand->line();
    qreal startAngle = atan2(curLine.y2() - curLine.y1(), curLine.x2() - curLine.x1());
    // 目标角度转atan2空间并归一化到[-π,π]
    qreal endAngle = (targetHour * 30.0 - 90.0) * M_PI / 180.0;
    while (endAngle > M_PI)  endAngle -= 2 * M_PI;
    while (endAngle < -M_PI) endAngle += 2 * M_PI;
    qreal handLen = curLine.length();
    int totalSteps = 50;
    int stepMs = 30;
    // 最短路径角度差
    qreal diff = endAngle - startAngle;
    while (diff > M_PI)  diff -= 2 * M_PI;
    while (diff < -M_PI) diff += 2 * M_PI;

    auto *timer = new QTimer(this);
    auto *step = new int(0);
    connect(timer, &QTimer::timeout, this, [=]() mutable {
        (*step)++;
        qreal t = qreal(*step) / totalSteps;
        // ease in-out
        qreal eased = t < 0.5 ? 2 * t * t : 1 - pow(-2 * t + 2, 2) / 2;
        qreal angle = startAngle + diff * eased;
        m_clockHand->setLine(cc.x(), cc.y(),
                             cc.x() + handLen * cos(angle),
                             cc.y() + handLen * sin(angle));
        if (*step >= totalSteps) {
            timer->stop();
            timer->deleteLater();
            delete step;
            if (onDone) onDone();
        }
    });
    timer->start(stepMs);
}

// 批量淡出
void GameScene::fadeOutItems(const QList<QPoint> &positions, std::function<void()> onDone) {
    QList<QPropertyAnimation*> anims;
    QList<QPointer<QGraphicsTextItem>> toDelete;

    for (const auto &gp : positions) {
        QGraphicsTextItem *item = itemAtGrid(gp);
        if (!item) continue;
        toDelete.append(item);
        m_pushables.removeOne(item);
        m_waters.removeOne(item);
        m_specialItems.removeOne(item);
        m_traps.removeOne(item);
        anims.append(Anim::prop(this, item, "opacity", 500, 0.0));
    }

    Anim::parallel(this, anims, [=]() {
        for (const auto &ptr : toDelete) {
            if (!ptr) continue;                    // 已被其他地方删除，跳过
            if (ptr->scene())
                removeItem(ptr);
            delete ptr;
        }
        if (onDone) onDone();
    });
}

// 方向箭头
void GameScene::updateArrow() {
    if (!m_arrow || !m_player) return;
    m_arrow->setPlainText(arrowCharForDir(m_dirX, m_dirY));
    QPointF playerPos = m_player->pos();
    m_arrow->setPos(playerPos + QPointF(m_dirX * Grid::kSize, m_dirY * Grid::kSize));
}

// 删除
void GameScene::deleteForward() {
    if (!m_player || !m_inputEnabled) return;
    if (m_currentLevel == 10) return;  // Level 10 has no delete mechanic
    if (m_deleteEffect->isPlaying()) return;

    QPoint playerGrid = Grid::toGrid(m_player->pos());
    QPoint targetGrid = playerGrid + QPoint(m_dirX, m_dirY);
    QGraphicsTextItem *targetItem = itemAtGrid(targetGrid);
    if (!targetItem) return;

    //第七关：删除树上的"没"字
    if (m_currentLevel == 7 && m_level7->isDeletableTreeChar(targetItem)) {
        m_inputEnabled = false;
        // 先移出列表再删除，否则 waters 中残留悬空指针
        m_waters.removeOne(targetItem);
        m_pushables.removeOne(targetItem);
        m_specialItems.removeOne(targetItem);
        m_deleteEffect->play(m_player, targetItem, [this, targetGrid]() {
            m_pushables.removeOne(nullptr);
            m_specialItems.removeOne(nullptr);
            m_waters.removeOne(nullptr);

            m_level7->bloomTreeAt(targetGrid, m_waters);
            // 不在此处恢复输入——若三棵全开，allBloomed 信号会触发
            // startPostBloomSequence 接管完整流程
            if (!m_level7->allTreesBloomed())
                m_inputEnabled = true;
        });
        return;
    }

    //第八关：交互对话中 Backspace 删除"不"字（石头破碎）
    if (m_currentLevel == 8
        && targetItem->toPlainText() == QStringLiteral("不")
        && Grid::toGrid(targetItem->pos()).y() == 13) {
        m_deleteEffect->play(m_player, targetItem, [this, targetItem]() {
            m_pushables.removeAll(targetItem);
            m_waters.removeAll(targetItem);
            if (m_interactionSystem)
                m_interactionSystem->notifyItemDeleted(targetItem);
            m_level8->tryStoneBreak(m_pushables, m_waters,
                                     m_inputEnabled, m_player);
        });
        return;
    }

    //默认：删除胜利目标
    if (targetItem->toPlainText() == m_winTarget) {
        m_deleteEffect->play(m_player, targetItem, [this]() {
            m_pushables.removeOne(nullptr);
            m_specialItems.removeOne(nullptr);
            m_waters.removeOne(nullptr);
            checkLevelComplete();
        });
    }
}

// Enter 键（第七关恩特之盾）
void GameScene::handleEnterKey() {
    if (m_currentLevel == 7)
        m_level7->handleEnterPress();
    else if (m_currentLevel == 9)
        m_level9->handleEnterPress();
    else if (m_currentLevel == 10)
        m_level10->handleEnterPress();
}

// F 键（交互）
bool GameScene::isInteractionActive() const {
    return m_interactionSystem && m_interactionSystem->isActive();
}

void GameScene::interactionAdvance() {
    if (m_interactionSystem && m_interactionSystem->isActive())
        m_interactionSystem->advance(m_pushables, m_waters, m_inputEnabled);
}

void GameScene::handleFKey() {
    if (m_interactionSystem) {
        m_interactionSystem->tryInteract(m_player, m_arrow,
                                          m_pushables, m_waters, m_inputEnabled);
    }
}

// 通关检测
void GameScene::checkLevelComplete() {
    if (m_levelPassed) return;
    if (m_currentLevel == 7 || m_currentLevel == 8 || m_currentLevel == 9 || m_currentLevel == 10 || m_currentLevel == 11 || m_currentLevel == 12) return;  // L7-L12 由信号控制通关

    qDebug() << "[GameScene] checkLevelComplete() level=" << m_currentLevel
             << "mechanic=" << (int)m_mechanic << "winCondition=" << (int)m_winCondition
             << "winTarget=" << m_winTarget;

    if (m_mechanic == LevelData::Standard) {
        if (m_winCondition == LevelData::HasText) {
            for (auto *item : items()) {
                if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
                    if (ti->toPlainText() == m_winTarget) {
                        qDebug() << "[GameScene] WIN TRIGGERED! m_winTarget matched:" << m_winTarget;
                        m_levelPassed = true;
                        m_inputEnabled = false;
                        QTimer::singleShot(400, this, [this]() {
                            qDebug() << "[GameScene] emit levelComplete() from checkLevelComplete (HasText)";
                            emit levelComplete();
                        });
                        return;
                    }
                }
            }
        } else if (m_winCondition == LevelData::NoText) {
            bool found = false;
            for (auto *item : items()) {
                if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
                    if (ti->toPlainText() == m_winTarget) { found = true; break; }
                }
            }
            if (!found) {
                m_levelPassed = true;
                m_inputEnabled = false;
                if (m_hasWinAnimation && !m_winFrameWords.isEmpty()) {
                    int minX = 1000, minY = 1000, maxX = -1, maxY = -1;
                    for (const auto &gp : m_winFrameWords) {
                        QPointF pixel = Grid::toPixel(gp.x(), gp.y());
                        if (pixel.x() < minX) minX = pixel.x();
                        if (pixel.y() < minY) minY = pixel.y();
                        if (pixel.x() + Grid::kSize > maxX) maxX = pixel.x() + Grid::kSize;
                        if (pixel.y() + Grid::kSize > maxY) maxY = pixel.y() + Grid::kSize;
                    }
                    const int padding = 5;
                    QRectF boxRect(minX - padding, minY - padding,
                                   (maxX - minX) + 2 * padding, (maxY - minY) + 2 * padding);
                    QGraphicsRectItem *frame = addRect(boxRect, QPen(Qt::white, 3));
                    frame->setOpacity(0.0);

                    QTimer *timer = new QTimer(this);
                    timer->setInterval(20);
                    connect(timer, &QTimer::timeout, this, [frame, timer, this]() {
                        qreal o = frame->opacity() + 0.04;
                        if (o >= 1.0) { o = 1.0; timer->stop(); timer->deleteLater(); emit levelComplete(); }
                        frame->setOpacity(o);
                    });
                    timer->start();
                } else {
                    QTimer::singleShot(400, this, [this]() { emit levelComplete(); });
                }
            }
        }
    }
}

// 自动行走
void GameScene::startAutoWalk(int targetX, int targetY) {
    m_autoWalkTarget = QPoint(targetX, targetY);
    m_autoWalkPath.clear();
    if (m_autoWalkTimer) { m_autoWalkTimer->stop(); delete m_autoWalkTimer; }

    QPoint from = Grid::toGrid(m_player->pos());
    m_autoWalkPath = findAutoWalkPath(from, m_autoWalkTarget);

    m_autoWalkTimer = new QTimer(this);
    connect(m_autoWalkTimer, &QTimer::timeout, this, &GameScene::autoWalkStep);
    m_autoWalkTimer->start(160);
    autoWalkStep();
}

void GameScene::autoWalkStep() {
    if (!m_player) { cancelAutoWalk(); return; }
    QPoint current = Grid::toGrid(m_player->pos());
    if (current == m_autoWalkTarget) {
        cancelAutoWalk();
        onAutoWalkComplete();
        return;
    }

    // 从预计算路径中取下一个格子；路径为空时回退直线移动
    QPoint nextGrid;
    if (!m_autoWalkPath.isEmpty()) {
        nextGrid = m_autoWalkPath.takeFirst();
        // 路径可能因推箱子变化而失效，校验相邻性
        int ddx = nextGrid.x() - current.x();
        int ddy = nextGrid.y() - current.y();
        if (qAbs(ddx) + qAbs(ddy) != 1) {
            // 路径失效，清空，改用直线逻辑
            m_autoWalkPath.clear();
        }
    }
    if (m_autoWalkPath.isEmpty()) {
        int diffX = m_autoWalkTarget.x() - current.x();
        int diffY = m_autoWalkTarget.y() - current.y();
        if (diffX != 0)      nextGrid = current + QPoint((diffX > 0) ? 1 : -1, 0);
        else if (diffY != 0) nextGrid = current + QPoint(0, (diffY > 0) ? 1 : -1);
        else { cancelAutoWalk(); onAutoWalkComplete(); return; }
    }

    int dx = nextGrid.x() - current.x();
    int dy = nextGrid.y() - current.y();
    m_dirX = dx; m_dirY = dy;
    m_arrow->setPlainText(arrowCharForDir(dx, dy));

    QPointF playerTarget = Grid::toPixel(nextGrid.x(), nextGrid.y());
    QPointF arrowTarget = playerTarget + QPointF(dx * Grid::kSize, dy * Grid::kSize);

    Anim::parallel(this, {
        Anim::prop(this, m_player, "pos", 120, playerTarget),
        Anim::prop(this, m_arrow, "pos", 120, arrowTarget)
    });
}

QList<QPoint> GameScene::findAutoWalkPath(QPoint from, QPoint to) {
    const int W = 20, H = 15;
    bool visited[20][15] = {};
    QPoint cameFrom[20][15];
    QQueue<QPoint> queue;
    const QList<QPoint> dirs = {{0,-1}, {0,1}, {-1,0}, {1,0}};

    queue.enqueue(from);
    visited[from.x()][from.y()] = true;

    while (!queue.isEmpty()) {
        QPoint cur = queue.dequeue();
        if (cur == to) {
            QList<QPoint> path;
            QPoint p = to;
            while (p != from) {
                path.prepend(p);
                p = cameFrom[p.x()][p.y()];
            }
            return path;
        }
        for (const auto &d : dirs) {
            QPoint next = cur + d;
            int nx = next.x(), ny = next.y();
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            if (visited[nx][ny]) continue;
            // 允许穿过目标格子（即使有障碍物）
            if (next != to && (isBlockedByWater(next) || isBlockedByPushable(next)))
                continue;
            // 也检查特殊物品
            if (next != to) {
                bool blocked = false;
                for (auto *sp : m_specialItems) {
                    if (Grid::toGrid(sp->pos()) == next) { blocked = true; break; }
                }
                if (blocked) continue;
            }
            visited[nx][ny] = true;
            cameFrom[nx][ny] = cur;
            queue.enqueue(next);
        }
    }
    // 无路可走时返回空
    return {};
}

void GameScene::cancelAutoWalk() {
    if (m_autoWalkTimer) { m_autoWalkTimer->stop(); delete m_autoWalkTimer; m_autoWalkTimer = nullptr; }
    m_autoWalkPath.clear();
}

void GameScene::onAutoWalkComplete() {
    if (m_currentLevel == 1) {
        QTimer::singleShot(300, this, [this]() {
            fadeTransition(2);
        });
    } else if (m_currentLevel == 2) {
        m_dialogueSystem->display("叔叔，来碗鸡汤面！", 13, 11, 22, [this]() {
            m_dialogueSystem->display("太好了，买到鸡汤面了！", 13, 11, 22, [this]() {
                fadeTransition(3);
            });
        });
    } else if (m_currentLevel == 8) {
        QTimer::singleShot(400, this, [this]() {
            fadeTransition(9);
        });
    }
}

// 渐隐过渡
void GameScene::fadeTransition(int nextLevel) {
    m_inputEnabled = false;
    m_moving = true;

    SaveManager::save("auto", collectSaveData());
    QString slot = "slot" + QString::number(m_currentLevel);
    SaveManager::save(slot, collectSaveData());

    Anim::blackout(this, 500, [=](QGraphicsRectItem *overlay) {
        cleanupLevelSpecific();
        if (nextLevel >= 1 && nextLevel <= 12) {
            removeItem(overlay);
            loadLevel(nextLevel);
            addItem(overlay);
            overlay->setZValue(999);
        }
    }, [=]() {
        if (nextLevel >= 1 && nextLevel <= 12) {
            m_moving = false;
        } else {
            m_moving = false;
            qDebug() << "[GameScene] emit levelComplete() from fadeTransition, nextLevel=" << nextLevel;
            emit levelComplete();
        }
    });
}

// 提示闪烁
void GameScene::createFlashingHint(const QString &text, int gx, int gy, int fontSize) {
    m_hintItem = addText(text, QFont("黑体", fontSize));
    m_hintItem->setDefaultTextColor(Qt::white);
    m_hintItem->setPos(Grid::toPixel(gx, gy));

    m_hintTimer = new QTimer(this);
    int *tick = new int(0);
    connect(m_hintTimer, &QTimer::timeout, this, [this, tick]() mutable {
        if (!m_hintItem) return;
        (*tick)++;
        qreal alpha = 0.625 + 0.375 * std::sin((*tick) * 0.05);
        m_hintItem->setOpacity(alpha);
    });
    m_hintTimer->start(40);
}

void GameScene::destroyHint() {
    if (m_hintTimer) { m_hintTimer->stop(); delete m_hintTimer; m_hintTimer = nullptr; }
    if (m_hintItem) { removeItem(m_hintItem); delete m_hintItem; m_hintItem = nullptr; }
}

// GameOver
void GameScene::triggerGameOver(const QString &message) {
    m_inputEnabled = false;
    m_dialogueSystem->display(message, 5, 10, 16, [this]() {
        int lvl = m_currentLevel;
        Anim::blackout(this, 400, [=](QGraphicsRectItem *overlay) {
            removeItem(overlay);
            loadLevel(lvl);
            addItem(overlay);
            overlay->setZValue(999);
        });
    });
}

// 二选一系统
void GameScene::showChoice(const QString &prompt, const QString &opt0, const QString &opt1,
                            std::function<void(int)> callback) {
    m_choiceActive = true;
    m_choiceIndex = 0;
    m_choiceCallback = std::move(callback);

    QFont pf("黑体", 20);
    auto *pt = addText(prompt, pf);
    pt->setDefaultTextColor(QColor(255, 255, 200));
    pt->setPos(Grid::toPixel(3, 4));
    m_choiceItems.append(pt);

    QFont of("黑体", 22);
    auto *o0 = addText("▶ " + opt0, of);
    o0->setDefaultTextColor(QColor(100, 255, 100));
    o0->setPos(Grid::toPixel(5, 6));
    m_choiceItems.append(o0);

    auto *o1 = addText("    " + opt1, of);
    o1->setDefaultTextColor(QColor(180, 180, 180));
    o1->setPos(Grid::toPixel(5, 8));
    m_choiceItems.append(o1);
}

void GameScene::choiceKeyPress(int key) {
    if (!m_choiceActive || m_choiceItems.size() < 3) return;

    if (key == Qt::Key_W && m_choiceIndex > 0) {
        m_choiceIndex = 0;
    } else if (key == Qt::Key_S && m_choiceIndex < 1) {
        m_choiceIndex = 1;
    } else if (key == Qt::Key_E) {
        m_choiceActive = false;
        int chosen = m_choiceIndex;
        auto cb = m_choiceCallback;
        clearChoice();
        if (cb) cb(chosen);
        return;
    } else {
        return;
    }

    // 更新高亮（先剥离前缀，避免累积位移）
    QColor sel(100, 255, 100), dim(180, 180, 180);
    auto *o0 = m_choiceItems[1];
    auto *o1 = m_choiceItems[2];
    auto stripPrefix = [](const QString &s) -> QString {
        if (s.startsWith("▶ ")) return s.mid(2);
        if (s.startsWith("    ")) return s.mid(4);
        return s;
    };
    QString t0 = stripPrefix(o0->toPlainText());
    QString t1 = stripPrefix(o1->toPlainText());
    if (m_choiceIndex == 0) {
        o0->setPlainText("▶ " + t0);
        o0->setDefaultTextColor(sel);
        o1->setPlainText("    " + t1);
        o1->setDefaultTextColor(dim);
    } else {
        o0->setPlainText("    " + t0);
        o0->setDefaultTextColor(dim);
        o1->setPlainText("▶ " + t1);
        o1->setDefaultTextColor(sel);
    }
}

void GameScene::clearChoice() {
    m_choiceActive = false;
    m_choiceIndex = 0;
    for (auto *item : m_choiceItems) {
        if (item) {
            if (item->scene())           // 避免已从场景移除的 item 被重复 removeItem
                removeItem(item);
            delete item;
        }
    }
    m_choiceItems.clear();
    m_choiceCallback = nullptr;
}

// 暂停菜单
bool GameScene::isPaused() const {
    return m_paused;
}

void GameScene::togglePause() {
    if (m_paused) {
        m_pauseMenu->close();
        m_paused = false;
        m_inputEnabled = true;
        if (m_platformerMechanic->isActive())
            m_platformerMechanic->resume();
        if (m_arrow) m_arrow->setVisible(m_arrowEnabled);
        m_player->setPos(m_savedPlayerPos);
        m_dirX = m_savedDirX; m_dirY = m_savedDirY;
        updateArrow();
        SaveManager::save("auto", collectSaveData());
    } else {
        if (!m_player) return;
        m_paused = true;
        m_inputEnabled = false;
        if (m_platformerMechanic->isActive())
            m_platformerMechanic->pause();
        if (m_arrow) m_arrow->setVisible(false);
        m_savedPlayerPos = m_player->pos();
        m_savedDirX = m_dirX; m_savedDirY = m_dirY;
        SaveManager::save("auto", collectSaveData());
        m_pauseMenu->setCurrentLevel(m_currentLevel);
        m_pauseMenu->open(m_player);
    }
}

void GameScene::pauseKeyPress(int key) {
    m_pauseMenu->handleKeyPress(key);
}

// 对话系统（转发）
bool GameScene::isDialogueActive() const {
    return m_dialogueSystem->isActive();
}

void GameScene::dialogueAdvance() {
    m_dialogueSystem->advance();
}

void GameScene::showDialogueSequence(const QStringList &lines, std::function<void()> onDone) {
    m_dialogueSystem->showSequence(lines, std::move(onDone));
}

void GameScene::showInteractiveText(const QString &text, int startGx, int gy,
                                     const QSet<int> &pushableIndices) {
    QList<QGraphicsTextItem*> chars = m_dialogueSystem->placeInteractiveText(text, startGx, gy, pushableIndices);
    for (int i = 0; i < chars.size(); ++i) {
        if (pushableIndices.contains(i)) {
            m_pushables.append(chars[i]);
        }
    }
}

// 钓鱼（转发）
void GameScene::fishingDropRope() {
    m_fishingMechanic->dropRope(m_player, m_inputEnabled, m_inputEnabled);
}

void GameScene::fishingCleanup() {
    m_fishingMechanic->cleanup();
}

// 平台跳跃（转发）
void GameScene::platformerKeyPress(int key) {
    m_platformerMechanic->keyPress(key);
}

void GameScene::platformerKeyRelease(int key) {
    m_platformerMechanic->keyRelease(key);
}

// Level 10 按键（转发）
void GameScene::level10KeyPress(int key) {
    if (m_level10) m_level10->keyPress(key);
}

void GameScene::level10KeyRelease(int key) {
    if (m_level10) m_level10->keyRelease(key);
}

// 摇摆装饰
QGraphicsTextItem *GameScene::addSwayingDecor(const QString &text, int gx, int gy) {
    QFont f("黑体", 24);
    QGraphicsTextItem *item = addText(text, f);
    item->setDefaultTextColor(Qt::white);
    item->setPos(Grid::toPixel(gx, gy));

    auto *sway = Anim::sway(this, item, 4.0, 1500);
    if (sway) m_decorAnims.append(sway);
    return item;
}

// 重置关卡
void GameScene::resetLevel() {
    loadLevel(m_currentLevel);   // 箭头可见性已由 loadLevel 按关卡正确设置
}

// 存档
SaveData GameScene::collectSaveData() const {
    SaveData d;
    d.level = m_currentLevel;
    if (m_player) {
        QPoint g = Grid::toGrid(m_player->pos());
        d.playerGx = g.x(); d.playerGy = g.y();
    }
    d.dirX = m_dirX; d.dirY = m_dirY;
    for (auto *p : m_pushables) {
        QPoint g = Grid::toGrid(p->pos());
        d.pushables.append({g.x(), g.y(), p->toPlainText()});
    }
    d.currentDialogueIndex = 0;
    return d;
}

// 文字逐个揭示
void GameScene::startReveal(const QList<RevealChar> &chars, std::function<void()> onDone) {
    if (m_revealTimer) {
        m_revealTimer->stop();
        disconnect(m_revealTimer, nullptr, this, nullptr);
        delete m_revealTimer;
        m_revealTimer = nullptr;
    }

    m_revealQueue = chars;
    m_revealIndex = 0;
    m_isRevealing = true;
    m_revealDoneCb = std::move(onDone);
    hideRevealCursor();

    m_revealTimer = new QTimer(this);
    connect(m_revealTimer, &QTimer::timeout, this, &GameScene::revealStep);
    m_revealTimer->start(90);
}

void GameScene::revealStep() {
    if (m_revealIndex >= m_revealQueue.size()) {
        m_revealTimer->stop();
        m_isRevealing = false;
        if (m_revealDoneCb) {
            auto cb = m_revealDoneCb;
            m_revealDoneCb = nullptr;
            cb();
        } else {
            m_inputEnabled = true;
            if (m_arrow && m_arrowEnabled) m_arrow->setVisible(true);
            showRevealCursor();
        }
        return;
    }
    const auto &rc = m_revealQueue[m_revealIndex];
    QFont f("黑体", 22);
    QGraphicsTextItem *ch = addText(rc.text, f);
    ch->setDefaultTextColor(Qt::white);
    ch->setPos(Grid::toPixel(rc.gx, rc.gy));

    if (rc.pushable) {
        m_pushables.append(ch);
    } else {
        m_waters.append(ch);
    }
    m_revealIndex++;
}

void GameScene::skipReveal() {
    if (!m_isRevealing) return;
    m_revealTimer->stop();
    while (m_revealIndex < m_revealQueue.size())
        revealStep();
    revealStep(); // 触发终止条件：m_isRevealing=false, m_inputEnabled=true
}

void GameScene::revealAdvance() {
    if (m_isRevealing)
        skipReveal();
}

void GameScene::startTextReveal(const QString &text, int startGx, int gy,
                                 const QSet<int> &pushableIndices,
                                 std::function<void()> onDone) {
    QList<RevealChar> chars;
    for (int i = 0; i < text.size(); ++i) {
        chars.append({QString(text[i]), startGx + i, gy,
                      pushableIndices.contains(i)});
    }
    startReveal(chars, std::move(onDone));
}

void GameScene::showRevealCursor() {
    if (m_revealQueue.isEmpty()) return;
    const auto &last = m_revealQueue.last();
    if (!m_revealCursor) {
        m_revealCursor = addText("▼", QFont("黑体", 12));
        m_revealCursor->setDefaultTextColor(Qt::white);
        m_revealCursor->setZValue(5);
    }
    m_revealCursor->setPos(Grid::toPixel(last.gx + 1, last.gy));
    m_revealCursor->setVisible(true);
}

void GameScene::hideRevealCursor() {
    if (m_revealCursor)
        m_revealCursor->setVisible(false);
}

// 关卡清理
void GameScene::cleanupLevelSpecific() {
    cancelAutoWalk();
    destroyHint();

    // 委托子系统清理
    m_fishingMechanic->cleanup();
    m_level3Road->cleanup();
    m_level4Classroom->cleanup();
    m_platformerMechanic->cleanup();
    m_level7->cleanup();
    m_level8->cleanup();
    m_level9->cleanup();
    m_level10->cleanup();
    m_level11->cleanup();
    m_level12->cleanup();
    if (m_deathEffect) m_deathEffect->cleanup();
    if (m_interactionSystem) m_interactionSystem->cleanup(m_pushables, m_waters);
    m_dialogueSystem->cleanup();
    m_level1->cleanup();
    m_level2->cleanup();

    // 装饰动画
    for (auto *a : m_decorAnims) { a->stop(); delete a; }
    m_decorAnims.clear();

    // 暂停菜单（若开着）
    m_pauseMenu->close();
    m_paused = false;

    // 揭示系统
    if (m_revealTimer) {
        m_revealTimer->stop();
        disconnect(m_revealTimer, nullptr, this, nullptr);
        delete m_revealTimer;
        m_revealTimer = nullptr;
    }
    if (m_revealCursor) {
        if (m_revealCursor->scene()) removeItem(m_revealCursor);
        delete m_revealCursor;
        m_revealCursor = nullptr;
    }
    m_revealQueue.clear();
    m_revealIndex = 0;
    m_isRevealing = false;

    // 清理遗留的命名定时器
    auto *mht = findChild<QTimer*>("menuHighlight");
    if (mht) { mht->stop(); delete mht; }

    m_inputEnabled = true;
    m_comboRules.clear();
    m_phraseRules.clear();
    m_traps.clear();
    m_trapSet.clear();
}
