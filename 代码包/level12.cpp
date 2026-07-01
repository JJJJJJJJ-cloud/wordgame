#include "level12.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "grid.h"

#include <QFont>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QVariantAnimation>
#include <QTimer>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QPen>
#include <QBrush>

Level12::Level12(QObject *parent) : LevelBase(parent) {}

//  入场
void Level12::startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                         bool &inputEnabled) {
    if (!player || !m_dialogueSystem || !m_scene) return;
    m_player = player;
    m_arrow = arrow;
    m_inputEnabled = &inputEnabled;
    *m_inputEnabled = false;
    if (m_arrow) m_arrow->setVisible(false);

    // 找到"门"
    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (!ti || ti == player) continue;
        if (ti->toPlainText() == QStringLiteral("门")
            && Grid::toGrid(ti->pos()) == QPoint(19, 12)) {
            m_doorItem = ti;
            break;
        }
    }

    QTimer::singleShot(500, this, [this]() {
        phase1_openingDialogue();
    });
}

//  阶段 1：开场对话
void Level12::phase1_openingDialogue() {
    if (!m_dialogueSystem) return;

    m_dialogueSystem->display(
        QStringLiteral("诶？所以到头来，只是一场梦..."),
        3, 12, 22, [this]() {
            m_dialogueSystem->display(
                QStringLiteral("我就说，这一切太离奇了..."),
                3, 12, 22, [this]() {
                    phase2_autoWalkToDoor();
                });
        });
}

//  阶段 2：自动行走 (2,10) 、 (16,10) 、 (16,12)
void Level12::phase2_autoWalkToDoor() {
    if (!m_player) return;

    walkTo(16, 10, [this]() {
        walkTo(16, 12, [this]() {
            phase3_roommateDialogue();
        });
    });
}

void Level12::walkTo(int gx, int gy, std::function<void()> onDone) {
    if (!m_player) { if (onDone) onDone(); return; }

    QPointF target = Grid::toPixel(gx, gy);
    QPointF from = m_player->pos();
    int dx = qAbs(gx - Grid::toGrid(from).x());
    int dy = qAbs(gy - Grid::toGrid(from).y());
    int duration = (dx + dy) * 110;

    auto *walk = new QPropertyAnimation(m_player, "pos");
    walk->setDuration(duration);
    walk->setEndValue(target);
    walk->setEasingCurve(QEasingCurve::InOutCubic);

    connect(walk, &QPropertyAnimation::finished, this, [onDone]() {
        if (onDone) onDone();
    });
    walk->start(QAbstractAnimation::DeleteWhenStopped);
}

//  阶段 3：舍友对话
void Level12::phase3_roommateDialogue() {
    if (!m_dialogueSystem) return;

    m_dialogueSystem->display(
        QStringLiteral("舍友还在打呼噜..."),
        3, 12, 22, [this]() {
            phase4_typewriterPush();
        });
}

//  阶段 4：打字机推人
void Level12::phase4_typewriterPush() {
    if (!m_player || !m_scene) return;

    m_pushText = QStringLiteral("舍友：呼噜呼噜呼噜呼噜呼噜呼噜呼噜呼");
    m_pushIndex = 0;
    m_pushChars.clear();

    m_pushTimer = new QTimer(this);
    m_tempTimers.append(m_pushTimer);
    connect(m_pushTimer, &QTimer::timeout, this, &Level12::pushStep);
    m_pushTimer->start(80);
}

void Level12::pushStep() {
    if (m_pushIndex >= m_pushText.size()) {
        m_pushTimer->stop();
        QTimer::singleShot(600, this, [this]() {
            phase5_fadeOut();
        });
        return;
    }

    if (!m_player || !m_scene) return;

    int gx = 3 + m_pushIndex;
    int gy = 12;
    QPoint charPos(gx, gy);

    // 检查是否与玩家位置冲突 、 推玩家向右
    QPoint playerGrid = Grid::toGrid(m_player->pos());
    if (charPos == playerGrid && playerGrid.x() < 19) {
        QPointF newPlayerPos = Grid::toPixel(playerGrid.x() + 1, gy);
        auto *pushAnim = new QPropertyAnimation(m_player, "pos");
        pushAnim->setDuration(80);
        pushAnim->setEndValue(newPlayerPos);
        pushAnim->setEasingCurve(QEasingCurve::OutCubic);
        pushAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QFont f(QStringLiteral("黑体"), 22);
    QString ch = QString(m_pushText[m_pushIndex]);
    auto *item = m_scene->addText(ch, f);
    item->setDefaultTextColor(Qt::white);
    item->setPos(Grid::toPixel(gx, gy));
    item->setZValue(10);
    m_pushChars.append(item);
    m_tempItems.append(item);

    m_pushIndex++;
}

//  阶段 5：淡出（进入梦境切换）
void Level12::phase5_fadeOut() {
    if (!m_player || !m_scene) {
        phase6_fadeInNewScene();
        return;
    }

    // "我" 渐隐
    auto *playerFade = new QPropertyAnimation(m_player, "opacity");
    playerFade->setDuration(800);
    playerFade->setEndValue(0.0);
    playerFade->setEasingCurve(QEasingCurve::InCubic);

    connect(playerFade, &QPropertyAnimation::finished, this, [this]() {
        if (!m_scene) { phase6_fadeInNewScene(); return; }

        // 全屏黑幕淡入
        auto *fadeRect = m_scene->addRect(m_scene->sceneRect(),
                                          QPen(Qt::NoPen),
                                          QBrush(Qt::black));
        fadeRect->setOpacity(0.0);
        fadeRect->setZValue(999);

        auto *screenFade = new QVariantAnimation(this);
        screenFade->setDuration(1200);
        screenFade->setStartValue(0.0);
        screenFade->setEndValue(1.0);
        screenFade->setEasingCurve(QEasingCurve::InQuad);

        connect(screenFade, &QVariantAnimation::valueChanged, this,
                [fadeRect](const QVariant &v) {
                    fadeRect->setOpacity(v.toDouble());
                });

        connect(screenFade, &QVariantAnimation::finished, this, [this]() {
            phase6_fadeInNewScene();
        });

        screenFade->start(QAbstractAnimation::DeleteWhenStopped);
    });

    playerFade->start(QAbstractAnimation::DeleteWhenStopped);
}

//  阶段 6：清空场景 、 渐亮到新场景
void Level12::phase6_fadeInNewScene() {
    if (!m_scene) return;

    // 清理所有旧物品
    m_player = nullptr;
    m_pushChars.clear();
    m_tempItems.clear();
    for (auto *t : m_tempTimers) { t->stop(); delete t; }
    m_tempTimers.clear();
    m_pushTimer = nullptr;

    // 清空场景
    m_scene->clear();

    // 加一层黑幕，然后渐亮
    auto *fadeRect = m_scene->addRect(m_scene->sceneRect(),
                                      QPen(Qt::NoPen),
                                      QBrush(Qt::black));
    fadeRect->setZValue(999);

    auto *fadeIn = new QVariantAnimation(this);
    fadeIn->setDuration(1000);
    fadeIn->setStartValue(1.0);
    fadeIn->setEndValue(0.0);
    fadeIn->setEasingCurve(QEasingCurve::OutQuad);

    connect(fadeIn, &QVariantAnimation::valueChanged, this,
            [fadeRect](const QVariant &v) {
                fadeRect->setOpacity(v.toDouble());
            });

    connect(fadeIn, &QVariantAnimation::finished, this, [this, fadeRect]() {
        m_scene->removeItem(fadeRect);
        delete fadeRect;
        phase7_slideInDreamChars();
    });

    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
}

//  阶段 7："呼噜呼噜我" 从左边滑入 (0,12)~(4,12)
void Level12::phase7_slideInDreamChars() {
    if (!m_scene) return;

    // 五字：呼噜呼噜我，目标(0,12)~(4,12)
    QStringList chars = {"呼", "噜", "呼", "噜", "我"};
    m_player = nullptr;  // 将在创建新"我"时更新

    for (int i = 0; i < chars.size(); ++i) {
        // 起始位置在屏幕左侧之外，每个字稍作错开
        qreal startX = -200.0 - i * 50.0;
        int delayMs = i * 80;  // 依次延迟进入
        auto *item = slideInChar(chars[i], i, 12, startX, delayMs);
        m_newSceneItems.append(item);
        if (chars[i] == QStringLiteral("我")) {
            m_player = item;
        }
    }

    // 等待所有字滑入完毕，然后进入阶段 8
    int totalDelay = 80 * 4 + 800;  // 最后一个字的延迟 + 动画时长
    QTimer::singleShot(totalDelay, this, [this]() {
        phase8_slideInTeacher();
    });
}

QGraphicsTextItem *Level12::slideInChar(const QString &ch, int targetGx, int gy,
                                        qreal startX, int delayMs,
                                        std::function<void()> onDone) {
    QFont f(QStringLiteral("黑体"), 22);
    auto *item = m_scene->addText(ch, f);
    item->setDefaultTextColor(Qt::white);
    item->setPos(startX, Grid::toPixel(0, gy).y());
    item->setZValue(10);

    QPointF target = Grid::toPixel(targetGx, gy);

    auto *anim = new QPropertyAnimation(item, "pos");
    anim->setDuration(700);
    anim->setStartValue(QPointF(startX, Grid::toPixel(0, gy).y()));
    anim->setEndValue(target);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    if (onDone) {
        connect(anim, &QPropertyAnimation::finished, this, [onDone]() {
            onDone();
        });
    }

    // 延迟启动
    QTimer::singleShot(delayMs, this, [anim]() {
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    });

    return item;
}

//  阶段 8："师" 从右边滑入到 (6,12)
void Level12::phase8_slideInTeacher() {
    if (!m_scene) { phase9_dreamDialogue(); return; }

    qreal startX = 900.0;
    auto *teacher = slideInChar(QStringLiteral("师"), 6, 12, startX, 0);
    m_newSceneItems.append(teacher);

    // 等待滑入完毕
    QTimer::singleShot(800, this, [this]() {
        phase9_dreamDialogue();
    });
}

//  阶段 9：对话
void Level12::phase9_dreamDialogue() {
    if (!m_dialogueSystem) { phase10_finalText(); return; }

    // 手动链式 display，每句在 (1, 9) 显示
    m_dialogueSystem->display(
        QStringLiteral("我：为什么..会被呼噜推出门..."),
        1, 9, 18, [this]() {
            m_dialogueSystem->display(
                QStringLiteral("老师：啊，这个字幕！"),
                1, 9, 18, [this]() {
                    m_dialogueSystem->display(
                        QStringLiteral("我能看到你的字幕！"),
                        1, 9, 18, [this]() {
                            m_dialogueSystem->display(
                                QStringLiteral("你拥有太以编辑的能力！"),
                                1, 9, 18, [this]() {
                                    shakeScreen();
                                    m_dialogueSystem->display(
                                        QStringLiteral("我：不要啊！我还没醒过来吗！"),
                                        1, 9, 18, [this]() {
                                            phase10_finalText();
                                        });
                                });
                        });
                });
        });
}

//  屏幕震动
void Level12::shakeScreen() {
    QGraphicsView *view = nullptr;
    for (auto *v : m_scene->views()) {
        view = qobject_cast<QGraphicsView*>(v);
        if (view) break;
    }
    if (!view) return;

    QTransform originalTransform = view->transform();

    auto *timer = new QTimer(this);
    int *tick = new int(0);
    const int totalTicks = 14;
    const int intensity = 40;

    connect(timer, &QTimer::timeout, this,
            [view, originalTransform, tick, totalTicks, intensity, timer]() {
        (*tick)++;
        if (*tick >= totalTicks) {
            timer->stop();
            view->setTransform(originalTransform);
            delete tick;
            timer->deleteLater();
            return;
        }
        qreal dx = (std::rand() % (intensity * 2 + 1)) - intensity;
        qreal dy = (std::rand() % (intensity * 2 + 1)) - intensity;
        QTransform shaken = originalTransform;
        shaken.translate(dx, dy);
        view->setTransform(shaken);
    });
    timer->start(25);
}

//  阶段 10：最终文字 、 黑屏 、 回主菜单
void Level12::phase10_finalText() {
    if (!m_scene || !m_dialogueSystem) {
        emit level12Complete();
        return;
    }

    // 黑屏
    auto *fadeRect = m_scene->addRect(m_scene->sceneRect(),
                                      QPen(Qt::NoPen),
                                      QBrush(Qt::black));
    fadeRect->setOpacity(0.0);
    fadeRect->setZValue(999);

    auto *screenFade = new QVariantAnimation(this);
    screenFade->setDuration(600);
    screenFade->setStartValue(0.0);
    screenFade->setEndValue(1.0);
    screenFade->setEasingCurve(QEasingCurve::InQuad);

    connect(screenFade, &QVariantAnimation::valueChanged, this,
            [fadeRect](const QVariant &v) {
                fadeRect->setOpacity(v.toDouble());
            });

    connect(screenFade, &QVariantAnimation::finished, this, [this, fadeRect]() {
        // 在黑屏上显示最终文字
        // 用 dialogueSystem 在黑幕之上显示
        if (!m_dialogueSystem) { emit level12Complete(); return; }

        m_dialogueSystem->setDisplayZValue(1000);
        m_dialogueSystem->setDisplayColor(Qt::white);
        m_dialogueSystem->display(
            QStringLiteral("你奇幻的贵校之旅，于此告一段落..."),
            1, 9, 18, [this]() {
                // 再保持黑屏一会，然后回主菜单
                QTimer::singleShot(1500, this, [this]() {
                    emit level12Complete();
                });
            });
    });

    screenFade->start(QAbstractAnimation::DeleteWhenStopped);
}

//  清理
void Level12::cleanup() {
    if (m_pushTimer) {
        m_pushTimer->stop();
    }

    for (auto *t : m_tempTimers) {
        t->stop();
        delete t;
    }
    m_tempTimers.clear();
    m_pushTimer = nullptr;

    // 旧场景物品（已在 phase6 通过 clear() 清除，这里仅清指针列表）
    m_tempItems.clear();
    m_pushChars.clear();

    // 新场景物品
    for (auto *item : m_newSceneItems) {
        if (item && item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    m_newSceneItems.clear();

    m_pushIndex = 0;
    m_pushText.clear();
    m_player = nullptr;
    m_arrow = nullptr;
    m_inputEnabled = nullptr;
    m_doorItem = nullptr;

    LevelBase::cleanup();
}
