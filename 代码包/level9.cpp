#include "level9.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "deleteeffect.h"
#include "deatheffect.h"
#include "grid.h"
#include "achievement.h"

#include <QFont>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QVariantAnimation>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <cmath>
#include <cstdlib>

Level9::Level9(QObject *parent) : LevelBase(parent) {}

//  入场序列
void Level9::startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                         bool &inputEnabled) {
    if (!player || !m_dialogueSystem) return;
    m_phase = Phase::Entry;
    m_hasDied = false;  // 成就 #9 初见杀：每次进入关卡重置死亡标记
    m_player = player;
    m_arrow = arrow;
    m_inputEnabledPtr = &inputEnabled;
    inputEnabled = false;
    if (arrow) arrow->setVisible(false);

    // 固定对话显示位置，防止继承上一关的 setSequencePos 状态
    m_dialogueSystem->setSequencePos(1, 12, 18);

    // 玩家从 (10,14) 的"门"处渐显
    player->setOpacity(0.0);
    player->setPos(Grid::toPixel(10, 14));

    // 收集恶龙字符 & 启动环境动画
    setupDragonAmbient();

    Anim::fade(this, player, 1.0, 800, [=, &inputEnabled]() {
        m_phase = Phase::WalkTo13;

        Anim::slide(this, player, Grid::toPixel(10, 13), 600, [=, &inputEnabled]() {
            m_phase = Phase::Dialogue1;
            QStringList lines = {
                QStringLiteral("半步踏进房间，根植于生命本能的寒栗便袭入骨髓。"),
                QStringLiteral("仿佛躯体先我一步接受了实力差距悬殊的现实。"),
                QStringLiteral("感知到我的出现，眼前的庞然大物慢慢睁开那骇人的双眸。"),
                QStringLiteral("龙：呵……又有不知死活的蝼蚁，踏入吾之领地了么……"),
            };
            playSequence(lines, [=, &inputEnabled]() {
                // 直接进入第二段对话（玩家留在 10,13，避免被对话挡住）
                m_phase = Phase::Dialogue2;
                QStringList lines2 = {
                    QStringLiteral("我：恶龙！停止你破坏校园的行径！"),
                    QStringLiteral("恶龙凝视我的眼神里多了几分轻蔑。"),
                    QStringLiteral("龙：汝...似乎并不知晓吾之名号。"),
                    QStringLiteral("吾名朗·昂索，自尔等对Wrong Answer之怖中擢升。"),
                    QStringLiteral("..."),
                    QStringLiteral("......"),
                    QStringLiteral("................")
                };
                playSequence(lines2, [=, &inputEnabled]() {
                    m_phase = Phase::Shake;
                    shakeScreen();

                    m_phase = Phase::Dialogue3;
                    QStringList lines3 = {
                        QStringLiteral("感觉DNA打结了。好中二。全方位的中二。"),
                        QStringLiteral("这种已经被玩烂的设定是怎么出现在贵校的啊！"),
                        QStringLiteral("那条龙没有任何一丝的羞耻心吗！"),
                        QStringLiteral("还有，居然是大家对WA的怨念催生了它吗。"),
                        QStringLiteral("谁家特级咒灵......"),
                        QStringLiteral("那你为什么要破坏我们的学校！尤其是机房！"),
                        QStringLiteral("朗·昂索：吾将摧毁此地的机器，从根源上终结尔等的痛苦。"),
                        QStringLiteral("我：所以它是想破坏所有电脑，来避免大家做题出现Wrong Answer...？"),
                        QStringLiteral("...好无懈可击的逻辑。从*根源*上解决问题了啊。")
                    };
                    playSequence(lines3, [=, &inputEnabled]() {
                        // 直接进入第四段对话（玩家留在 10,13）
                        m_phase = Phase::Dialogue4;
                        QStringList lines4 = {
                            QStringLiteral("但是，你的存在……本身就是对恐惧的误解！"),
                            QStringLiteral("克服恐惧的办法，不是毁掉所有可能犯错的机会！"),
                            QStringLiteral("朗·昂索：...住口！区区凡人，竟敢质疑吾之宏图？"),
                            QStringLiteral("多说无益，吾动动手指，便能让汝口紧闭！")
                        };
                        playSequence(lines4, [=, &inputEnabled]() {
                            m_phase = Phase::FreePlay;
                            inputEnabled = true;
                            if (arrow) {
                                QPointF pp = player->pos();
                                arrow->setPos(pp.x() + Grid::kSize, pp.y());
                                arrow->setPlainText(QStringLiteral("▶"));
                                arrow->setVisible(true);
                            }
                            QTimer::singleShot(800, this, [this]() {
                                if (m_phase == Phase::FreePlay)
                                    startBulletHell();
                            });
                        });
                    });
                });
            });
        });
    });
}

//  恶龙环境动画
void Level9::setupDragonAmbient() {
    if (!m_scene) return;
    m_dragonChars.clear();

    const QList<QPoint> dragonPositions = {
        {5,5},{6,4},{7,3},{8,0},{8,3},{9,0},{9,1},{9,3},{9,4},
        {9,5},{10,1},{10,2},{10,3},{10,4},{10,5},{10,6},{10,7},
        {11,0},{11,1},{11,3},{11,4},{11,5},{12,0},{12,3},{13,3},
        {14,4},{15,5}
    };

    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (!ti) continue;
        if (ti->toPlainText() == QStringLiteral("龙")
            && dragonPositions.contains(Grid::toGrid(ti->pos()))) {
            m_dragonChars.append(ti);
        }
    }

    // 每只"龙"微弱的呼吸式缩放
    for (auto *ti : m_dragonChars) {
        auto *anim = Anim::breathe(this, ti, 1.08, 3000 + (std::rand() % 1500));
        if (anim) m_dragonAnims.append(anim);
    }
}

//  屏幕震动
void Level9::shakeScreen() {
    QGraphicsView *view = m_view;
    if (!view) return;

    QTransform saved = view->transform();

    auto *shake = new QVariantAnimation(this);
    shake->setDuration(600);
    shake->setStartValue(0);
    shake->setEndValue(20);
    shake->setEasingCurve(QEasingCurve::Linear);

    connect(shake, &QVariantAnimation::valueChanged, this,
            [view, saved](const QVariant &v) {
        int t = v.toInt();
        qreal decay = (t < 14) ? 1.0 : 1.0 - (t - 14) / 6.0;
        if (decay < 0.0) decay = 0.0;
        qreal sx = ((std::rand() % 25) - 12) * decay;
        qreal sy = ((std::rand() % 21) - 10) * decay;
        view->setTransform(QTransform(saved).translate(sx, sy));
    });

    connect(shake, &QVariantAnimation::finished, this, [view, saved]() {
        view->setTransform(saved);
    });

    shake->start(QAbstractAnimation::DeleteWhenStopped);
}

//  白屏过渡：全屏渐白 + 持续抖动 、 关卡结束
void Level9::startWhiteOut() {
    m_phase = Phase::WhiteOut;
    if (m_inputEnabledPtr) *m_inputEnabledPtr = false;
    if (m_arrow) m_arrow->setVisible(false);

    if (!m_scene) {
        QTimer::singleShot(200, this, [this]() {
            m_phase = Phase::Done;
            emit level9Complete();
        });
        return;
    }

    // ① 全屏白色遮罩渐显
    QGraphicsRectItem *whiteRect = m_scene->addRect(m_scene->sceneRect(),
                                                     QPen(Qt::NoPen),
                                                     QBrush(Qt::white));
    whiteRect->setOpacity(0.0);
    whiteRect->setZValue(500);

    auto *fade = new QVariantAnimation(this);
    fade->setDuration(1200);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::InQuad);

    connect(fade, &QVariantAnimation::valueChanged, this,
            [whiteRect](const QVariant &v) {
        whiteRect->setOpacity(v.toDouble());
    });

    // ② 屏幕持续抖动（在白屏渐显期间）
    QGraphicsView *view = m_view;
    if (view) {
        QTransform saved = view->transform();

        auto *shake = new QVariantAnimation(this);
        shake->setDuration(1200);
        shake->setStartValue(0);
        shake->setEndValue(30);
        shake->setEasingCurve(QEasingCurve::Linear);

        connect(shake, &QVariantAnimation::valueChanged, this,
                [view, saved](const QVariant &v) {
            int t = v.toInt();
            qreal intensity = (t < 20) ? t / 20.0 : 1.0;
            qreal sx = ((std::rand() % 35) - 17) * intensity;
            qreal sy = ((std::rand() % 27) - 13) * intensity;
            view->setTransform(QTransform(saved).translate(sx, sy));
        });

        connect(shake, &QVariantAnimation::finished, this, [view, saved]() {
            view->setTransform(saved);
        });

        shake->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // ③ 白屏完全覆盖后 、 关卡结束
    connect(fade, &QVariantAnimation::finished, this, [this, whiteRect]() {
        QTimer::singleShot(300, this, [this, whiteRect]() {
            m_scene->removeItem(whiteRect);
            delete whiteRect;
            m_phase = Phase::Done;
            emit level9Complete();
        });
    });

    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

//  弹幕系统（向下移动，与 Level7 速度一致）
void Level9::startBulletHell() {
    if (!m_scene || !m_player) return;
    m_phase = Phase::BulletHell;
    m_bulletParried = false;
    m_bulletY = 8.0;
    m_bulletFadeTick = 0;
    m_bulletPhase = 0;

    QFont bulletFont(QStringLiteral("黑体"), 24, QFont::Bold);

    for (int x = 0; x <= 19; ++x) {
        auto *b = m_scene->addText(QStringLiteral("弹"), bulletFont);
        b->setDefaultTextColor(Qt::white);
        b->setPos(Grid::toPixel(x, 8));
        b->setOpacity(0.0);
        b->setZValue(30);

        auto *glow = new QGraphicsDropShadowEffect;
        glow->setBlurRadius(22);
        glow->setColor(QColor(255, 180, 40, 220));
        glow->setOffset(0, 0);
        b->setGraphicsEffect(glow);

        m_bullets.append(b);
    }

    if (m_bulletTimer) { m_bulletTimer->stop(); delete m_bulletTimer; }
    m_bulletTimer = new QTimer(this);
    connect(m_bulletTimer, &QTimer::timeout, this, [this]() { tickBullets(); });
    m_bulletTimer->start(50);
}

void Level9::tickBullets() {
    if (m_bulletPhase == 0) {
        m_bulletFadeTick++;
        qreal alpha = m_bulletFadeTick / 10.0;
        if (alpha > 1.0) alpha = 1.0;
        for (auto *b : m_bullets)
            if (b) b->setOpacity(alpha);
        if (alpha >= 1.0) {
            m_bulletFadeTick = 0;
            m_bulletPhase = 1;
        }
        return;
    }

    // 向下移动，速度与 Level7 一致（0.333 格/50ms 帧）
    const qreal step = 0.333;
    m_bulletY += step;

    for (int i = 0; i < m_bullets.size(); ++i) {
        auto *b = m_bullets[i];
        if (!b) continue;
        int gx = Grid::toGrid(b->pos()).x();
        b->setPos(gx * Grid::kSize, m_bulletY * Grid::kSize);
    }

    checkBulletPlayerCollision();

    // 全部移出屏幕下方
    if (m_bulletY >= 15.0) {
        for (auto *b : m_bullets) {
            if (!b || !b->scene()) continue;
            Anim::fade(this, b, 0.0, 400, [this, b]() {
                if (b && b->scene()) { b->scene()->removeItem(b); delete b; }
            });
        }
        m_bullets.clear();
        if (m_bulletTimer) { m_bulletTimer->stop(); delete m_bulletTimer; m_bulletTimer = nullptr; }

        if (!m_bulletParried && m_phase != Phase::Death) {
            onBulletAllPassed();
        }
    }
}

void Level9::checkBulletPlayerCollision() {
    if (!m_player || m_phase == Phase::Death) return;
    QPoint pg = Grid::toGrid(m_player->pos());

    for (int i = m_bullets.size() - 1; i >= 0; --i) {
        auto *b = m_bullets[i];
        if (!b) continue;
        QPoint bg = Grid::toGrid(b->pos());
        if (bg == pg) {
            triggerDeath();
            return;
        }
    }
}

//  Enter 键：恩特之盾弹反
void Level9::handleEnterPress() {
    if (m_phase != Phase::BulletHell || !m_player || !m_arrow || m_bulletParried)
        return;

    QPoint playerGrid = Grid::toGrid(m_player->pos());
    QPoint ad = arrowDir(m_arrow->toPlainText());
    QPoint frontCell = playerGrid + ad;

    for (int i = m_bullets.size() - 1; i >= 0; --i) {
        auto *b = m_bullets[i];
        if (!b) continue;
        QPoint bg = Grid::toGrid(b->pos());
        if (bg == frontCell) {
            m_bulletParried = true;
            if (m_deleteEffect)
                m_deleteEffect->playEnterShield(m_player);

            // 瞬移到身后一格（与 Level7 恩特之盾行为一致）
            QPoint behind = playerGrid - ad;
            b->setPos(behind.x() * Grid::kSize, behind.y() * Grid::kSize);
            b->setDefaultTextColor(QColor(100, 255, 200));
            m_bullets[i] = nullptr;

            // 被弹反的"弹"短暂显示绿色后渐隐消失
            QPointer<QGraphicsTextItem> parriedBullet(b);
            QTimer::singleShot(800, this, [this, parriedBullet]() {
                if (!parriedBullet || !parriedBullet->scene()) return;
                Anim::fade(this, parriedBullet, 0.0, 400, [this, parriedBullet]() {
                    if (parriedBullet && parriedBullet->scene()) {
                        parriedBullet->scene()->removeItem(parriedBullet);
                        delete parriedBullet;
                    }
                });
            });

            // 其余子弹消失
            QTimer::singleShot(300, this, [this]() {
                for (auto *bb : m_bullets) {
                    if (!bb || !bb->scene()) continue;
                    Anim::fade(this, bb, 0.0, 500, [this, bb]() {
                        if (bb && bb->scene()) { bb->scene()->removeItem(bb); delete bb; }
                    });
                }
                m_bullets.clear();
            });

            if (m_bulletTimer) { m_bulletTimer->stop(); delete m_bulletTimer; m_bulletTimer = nullptr; }

            onBulletParried();
            return;
        }
    }
}

void Level9::onBulletParried() {
    if (!m_player || !m_arrow || !m_inputEnabledPtr || !m_dialogueSystem) return;
    m_phase = Phase::Dialogue5;
    *m_inputEnabledPtr = false;
    if (m_arrow) m_arrow->setVisible(false);

    QStringList lines = {
        QStringLiteral("朗·昂索：那是何等僭越之物！不属于此世之力..."),
        QStringLiteral("即便如此...吾仍可弹指间将汝化作齑粉!")
    };
    playSequence(lines, [this]() {
        if (!m_player || !m_inputEnabledPtr || !m_watersPtr) return;
        m_phase = Phase::WalkBackTo13;
        QPointF backTarget = Grid::toPixel(10, 13);
        Anim::slide(this, m_player, backTarget, 700, [this]() {
            m_phase = Phase::Dialogue6;
            QStringList lines6 = {
                QStringLiteral("我：......"),
            };
            playSequence(lines6, [this]() {
                spawnRedText1(*m_watersPtr);
                spawnRedText2(*m_watersPtr);
            });
        }, QEasingCurve::InOutCubic);
    });
}

void Level9::onBulletAllPassed() {
    // 弹幕全部通过但未被弹反 、 仍触发后续（宽容处理）
    onBulletParried();
}

//  红色文字系统
void Level9::spawnRedText1(QList<QGraphicsTextItem*> &waters) {
    if (!m_scene) return;
    m_phase = Phase::RedText1;

    const QString text = QStringLiteral("吾与汝之力量岂可同日而语！");
    QFont f(QStringLiteral("黑体"), 24, QFont::Bold);
    QColor red(255, 60, 40);

    for (int i = 0; i < text.size(); ++i) {
        int gx = i;  // (0,9) 开始
        int gy = 9;
        auto *ch = m_scene->addText(QString(text[i]), f);
        ch->setDefaultTextColor(red);
        ch->setPos(Grid::toPixel(gx, gy));
        ch->setZValue(40);
        ch->setOpacity(0.0);

        ch->setOpacity(0.0);
        Anim::fade(this, ch, 1.0, 300);

        m_redText1Chars.append(ch);
        waters.append(ch);

        if (i == 6) m_deleteTarget1 = ch;  // "岂"
    }

    // 文字出现后，自动走两段路径并自动触发删除
    QTimer::singleShot(800, this, [this, &waters]() {
        if (!m_player || !m_arrow || !m_inputEnabledPtr) return;
        m_phase = Phase::AutoWalkDelete1;
        *m_inputEnabledPtr = false;
        if (m_arrow) m_arrow->setVisible(false);

        // 第一段：(10,13) 、 (6,13)
        autoWalkTo(QPoint(6, 13), [this, &waters]() {
            // 第二段：(6,13) 、 (6,10)
            autoWalkTo(QPoint(6, 10), [this, &waters]() {
                // 自动删除"岂"（使用 DeleteEffect 与 Level7 统一）
                auto *target = m_deleteTarget1;
                if (!target || !m_deleteEffect || !m_player) return;
                m_redText1Cleared = true;

                m_deleteEffect->play(m_player, target, [this, target, &waters]() {
                    m_redText1Chars.removeAll(target);
                    m_deleteTarget1 = nullptr;

                    // 剩余文字变白
                    for (auto *ch : m_redText1Chars)
                        if (ch && ch->scene()) ch->setDefaultTextColor(Qt::white);
                    clearRedText1Collision(waters);

                    //衔接第二段红字的删除
                    // 第三段：(6,10) 、 (13,10)
                    autoWalkTo(QPoint(13, 10), [this, &waters]() {
                        // 第四段：(13,10) 、 (13,11)
                        autoWalkTo(QPoint(13, 11), [this, &waters]() {
                            auto *target2 = m_deleteTarget2;
                            if (!target2 || !m_deleteEffect || !m_player) return;
                            m_redText2Cleared = true;

                            m_deleteEffect->play(m_player, target2, [this, target2, &waters]() {
                                m_redText2Chars.removeAll(target2);
                                m_deleteTarget2 = nullptr;

                                for (auto *ch : m_redText2Chars)
                                    if (ch && ch->scene()) ch->setDefaultTextColor(Qt::white);
                                clearRedText2Collision(waters);

                                // —— 最终对话 + 白屏过渡 ——
                                m_phase = Phase::FinalDialogue;
                                QTimer::singleShot(400, this, [this]() {
                                    if (!m_dialogueSystem) {
                                        startWhiteOut();
                                        return;
                                    }
                                    QStringList lines = {
                                        QStringLiteral("朗·昂索：居然有此等利器..."),
                                        QStringLiteral("哼……此地狭窄，施展不开。"),
                                        QStringLiteral("可敢随吾去个宽敞之处，"),
                                        QStringLiteral("堂堂正正地了结此战？"),
                                        QStringLiteral("我：你想做什么！")
                                    };
                                    playSequence(lines, [this]() {
                                        m_dialogueSystem->cleanup();
                                        startWhiteOut();
                                    });
                                });
                            });
                        });
                    });
                });
            });
        });
    });
}

void Level9::spawnRedText2(QList<QGraphicsTextItem*> &waters) {
    if (!m_scene) return;
    m_phase = Phase::RedText2;

    const QString text = QStringLiteral("汝有何手段阻拦吾！");
    QFont f(QStringLiteral("黑体"), 24, QFont::Bold);
    QColor red(255, 60, 40);

    for (int i = 0; i < text.size(); ++i) {
        int gx = 11 + i;  // (11,12) 开始
        int gy = 12;
        auto *ch = m_scene->addText(QString(text[i]), f);
        ch->setDefaultTextColor(red);
        ch->setPos(Grid::toPixel(gx, gy));
        ch->setZValue(40);
        ch->setOpacity(0.0);

        ch->setOpacity(0.0);
        Anim::fade(this, ch, 1.0, 300);

        m_redText2Chars.append(ch);
        waters.append(ch);

        if (i == 2) m_deleteTarget2 = ch;  // "何"
    }
}

void Level9::clearRedText1Collision(QList<QGraphicsTextItem*> &waters) {
    for (auto *ch : m_redText1Chars) {
        if (ch) waters.removeAll(ch);
    }
}

void Level9::clearRedText2Collision(QList<QGraphicsTextItem*> &waters) {
    for (auto *ch : m_redText2Chars) {
        if (ch) waters.removeAll(ch);
    }
}

//  Backspace 删除：拦截红色文字中的关键字
bool Level9::handleDeleteForward(QList<QGraphicsTextItem*> &pushables,
                                  QList<QGraphicsTextItem*> &waters,
                                  bool &inputEnabled,
                                  QGraphicsTextItem *player) {
    Q_UNUSED(pushables);
    Q_UNUSED(waters);
    Q_UNUSED(inputEnabled);
    Q_UNUSED(player);
    return false;  // 红色文字删除已由剧情自动处理
}

bool Level9::isDeletableTarget(QGraphicsTextItem *item) const {
    Q_UNUSED(item);
    return false;  // 删除由剧情自动触发，不依赖 Backspace
}

//  自动行走（重写基类）
void Level9::autoWalkStep() {
    if (!m_player) {
        if (m_autoWalkTimer) { m_autoWalkTimer->stop(); delete m_autoWalkTimer; m_autoWalkTimer = nullptr; }
        return;
    }

    QPoint cur = Grid::toGrid(m_player->pos());
    if (cur == m_autoWalkTarget) {
        if (m_autoWalkTimer) { m_autoWalkTimer->stop(); delete m_autoWalkTimer; m_autoWalkTimer = nullptr; }
        if (m_autoWalkDone) { auto cb = m_autoWalkDone; m_autoWalkDone = nullptr; cb(); }
        return;
    }

    int dx = 0, dy = 0;
    if (m_autoWalkTarget.x() > cur.x()) dx = 1;
    else if (m_autoWalkTarget.x() < cur.x()) dx = -1;
    else if (m_autoWalkTarget.y() > cur.y()) dy = 1;
    else if (m_autoWalkTarget.y() < cur.y()) dy = -1;

    QPoint next = cur + QPoint(dx, dy);
    QPointF targetPos = Grid::toPixel(next.x(), next.y());

    Anim::slide(this, m_player, targetPos, 120);
}

//  死亡系统（委托给 DeathEffect）
void Level9::triggerDeath() {
    if (m_phase == Phase::Death || !m_deathEffect) return;
    m_phase = Phase::Death;
    m_hasDied = true;  // 成就 #9 初见杀
    AchievementManager::instance()->unlock(AchievementId::ChuJianSha);
    if (m_inputEnabledPtr) *m_inputEnabledPtr = false;
    if (m_bulletTimer) { m_bulletTimer->stop(); delete m_bulletTimer; m_bulletTimer = nullptr; }

    for (auto *b : m_bullets) {
        if (b && b->scene()) { b->scene()->removeItem(b); delete b; }
    }
    m_bullets.clear();
    if (m_arrow) m_arrow->setVisible(false);

    QStringList deathDialogues = {
        QStringLiteral("额啊...还是难以匹敌吗..."),
        QStringLiteral("朗·昂索：如此显著的事实，尔等为何不懂？"),
        QStringLiteral("你的视线逐渐模糊..."),
        QStringLiteral("愤怒与不甘，成为谁人那早已失神的眼底最后的沉淀...")
    };

    QStringList epilogueLines = {
        QStringLiteral("你的旅途戛然而止。"),
        QStringLiteral("喧嚣的、奇幻的、异彩纷呈的一切，自你阖眼那一刻，"),
        QStringLiteral("便化作梦中稍纵即逝的泡影。"),
        QStringLiteral("但或许，你还有重新来过的机会。"),
        QStringLiteral("ESC键进入暂停页面，选择时间回溯，继续你的贵校旅程。")
    };

    m_deathEffect->play(m_player, deathDialogues, epilogueLines);
}

//  清理
void Level9::cleanup() {
    for (auto *a : m_dragonAnims) { a->stop(); delete a; }
    m_dragonAnims.clear();
    m_dragonChars.clear();

    if (m_bulletTimer) { m_bulletTimer->stop(); delete m_bulletTimer; m_bulletTimer = nullptr; }
    for (auto *b : m_bullets) {
        if (b && b->scene()) { b->scene()->removeItem(b); delete b; }
    }
    m_bullets.clear();

    if (m_autoWalkTimer) { m_autoWalkTimer->stop(); delete m_autoWalkTimer; m_autoWalkTimer = nullptr; }

    if (m_deathEffect) m_deathEffect->cleanup();

    // 清理红字（已在场景中的）
    for (auto *ch : m_redText1Chars) {
        if (ch && ch->scene()) { ch->scene()->removeItem(ch); delete ch; }
    }
    m_redText1Chars.clear();
    for (auto *ch : m_redText2Chars) {
        if (ch && ch->scene()) { ch->scene()->removeItem(ch); delete ch; }
    }
    m_redText2Chars.clear();
    m_deleteTarget1 = nullptr;
    m_deleteTarget2 = nullptr;
    m_redText1Cleared = false;
    m_redText2Cleared = false;

    m_bulletParried = false;
    m_bulletY = 8.0;
    m_bulletPhase = 0;
    m_bulletFadeTick = 0;

    m_player = nullptr;
    m_arrow = nullptr;
    m_inputEnabledPtr = nullptr;
    m_pushablesPtr = nullptr;
    m_watersPtr = nullptr;
    m_phase = Phase::Entry;

    LevelBase::cleanup();
}
