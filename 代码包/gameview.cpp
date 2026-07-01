#include "gameview.h"
#include "gamescene.h"
#include "leveldata.h"
#include <QKeyEvent>
#include <QScrollBar>
#include <QPainter>

GameView::GameView(QWidget *parent) : QGraphicsView(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setRenderHint(QPainter::Antialiasing);

    // 去掉细小白线，否则影响观感
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setBackgroundBrush(Qt::black);

    m_moveTimer = new QTimer(this);
    connect(m_moveTimer, &QTimer::timeout, this, &GameView::processMovement);
}

void GameView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    if (!m_zoomLocked)
        fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void GameView::keyPressEvent(QKeyEvent *event) {
    GameScene *myScene = qobject_cast<GameScene*>(scene());

    // ESC 切换暂停
    if (event->key() == Qt::Key_Escape) {
        if (myScene) myScene->togglePause();
        return;
    }

    // 暂停模式：所有按键转发给菜单
    if (myScene && myScene->isPaused()) {
        myScene->pauseKeyPress(event->key());
        return;
    }

    // Level 10: 打飞机模式
    if (myScene && myScene->isLevel10Active()) {
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            myScene->handleEnterKey();
            return;
        }
        if (event->key() == Qt::Key_E && !event->isAutoRepeat()) {
            if (myScene->isDialogueActive()) {
                myScene->dialogueAdvance();
                return;
            }
        }
        if (event->key() == Qt::Key_W || event->key() == Qt::Key_A ||
            event->key() == Qt::Key_S || event->key() == Qt::Key_D) {
            myScene->level10KeyPress(event->key());
            return;
        }
        return;
    }

    // Enter / Return 键：第七关恩特之盾规避
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (myScene) myScene->handleEnterKey();
        return;
    }

    // F 键：交互
    if (event->key() == Qt::Key_F) {
        if (myScene) myScene->handleFKey();
        return;
    }

    // 二选一模式
    if (myScene && myScene->isChoiceActive()) {
        if (event->key() == Qt::Key_W || event->key() == Qt::Key_S ||
            event->key() == Qt::Key_E) {
            myScene->choiceKeyPress(event->key());
        }
        return;
    }

    // 平台跳跃模式：E 键优先推进对话，其余按键转发
    if (myScene && myScene->currentMechanic() == LevelData::TowerClimb) {
        if (event->key() == Qt::Key_E && !event->isAutoRepeat()) {
            if (myScene && myScene->isDialogueActive()) {
                myScene->dialogueAdvance();
                return;
            }
        }
        if (!event->isAutoRepeat())
            myScene->platformerKeyPress(event->key());
        return;
    }

    // 对话激活时，E 键推进对话
    if (event->key() == Qt::Key_E) {
        if (myScene && myScene->isRevealing()) {
            myScene->revealAdvance();
            return;
        }
        if (myScene && myScene->isDialogueActive()) {
            myScene->dialogueAdvance();
            return;
        }
        if (myScene && myScene->isInteractionActive()) {
            myScene->interactionAdvance();
            return;
        }
    }

    if (event->key() == Qt::Key_Space) {
        if (myScene && myScene->currentMechanic() == LevelData::Fishing) {
            myScene->fishingDropRope();
            return;
        }
    }

    if (event->key() == Qt::Key_Backspace) {
        if (myScene) myScene->deleteForward();
        return;
    }

    m_keysHeld.insert(event->key());
    if (!m_moveTimer->isActive()) {
        m_moveTimer->start(150);
        processMovement();
    }
}

void GameView::keyReleaseEvent(QKeyEvent *event) {
    GameScene *myScene = qobject_cast<GameScene*>(scene());

    // 暂停模式：不处理释放
    if (myScene && myScene->isPaused()) return;

    // Level 10: 转发 WASD 释放
    if (myScene && myScene->isLevel10Active()) {
        if (event->key() == Qt::Key_W || event->key() == Qt::Key_A ||
            event->key() == Qt::Key_S || event->key() == Qt::Key_D) {
            myScene->level10KeyRelease(event->key());
        }
        return;
    }

    if (myScene && myScene->currentMechanic() == LevelData::TowerClimb) {
        if (!event->isAutoRepeat())
            myScene->platformerKeyRelease(event->key());
        return;
    }
    m_keysHeld.remove(event->key());
    if (!m_keysHeld.contains(Qt::Key_W) &&
        !m_keysHeld.contains(Qt::Key_A) &&
        !m_keysHeld.contains(Qt::Key_S) &&
        !m_keysHeld.contains(Qt::Key_D)) {
        m_moveTimer->stop();
    }
}

void GameView::processMovement() {
    GameScene *myScene = qobject_cast<GameScene*>(scene());
    if (!myScene) return;

    int dx = 0, dy = 0;
    if (m_keysHeld.contains(Qt::Key_W)) dy -= 1;
    if (m_keysHeld.contains(Qt::Key_S)) dy += 1;
    if (m_keysHeld.contains(Qt::Key_A)) dx -= 1;
    if (m_keysHeld.contains(Qt::Key_D)) dx += 1;

    // 禁止斜向移动：如果 dx 和 dy 都不为0，则不移动
    if (dx != 0 && dy != 0) {
        return; // 或选择忽略 dx 或 dy，这里直接放弃移动
    }

    if (dx != 0 || dy != 0) {
        myScene->movePlayer(dx, dy);
    }
}