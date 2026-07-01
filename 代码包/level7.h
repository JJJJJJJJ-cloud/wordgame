#ifndef LEVEL7_H
#define LEVEL7_H

#include "levelbase.h"

#include <QAbstractAnimation>
#include <QList>
#include <QPoint>
#include <QPointer>
#include <QTimer>

class DialogueSystem;
class DeleteEffect;

class Level7 : public LevelBase
{
    Q_OBJECT
public:
    explicit Level7(QObject *parent = nullptr);

    // 入场序列
    void startEntry(QGraphicsTextItem *player, QGraphicsTextItem *teacher,
                    QGraphicsTextItem *arrow, bool &inputEnabled);

    // 树摇曳 + 检测 / 开花
    void setupTrees();
    bool isDeletableTreeChar(QGraphicsTextItem *item) const;
    void bloomTreeAt(QPoint gridPos, QList<QGraphicsTextItem*> &waters);
    bool allTreesBloomed() const { return m_bloomedCount >= 3; }

    // Enter 键（恩特之盾规避）
    void handleEnterPress();

    // 三棵树开花后 、 转场 、 恩特之盾教学 、 弹幕
    void startPostBloomSequence(QGraphicsTextItem *player,
                                QGraphicsTextItem *teacher,
                                QGraphicsTextItem *arrow,
                                QList<QGraphicsTextItem*> &waters,
                                bool &inputEnabled);

    void cleanup() override;

signals:
    void entryComplete();
    void allBloomed();
    void bulletHellComplete();
    void trainingComplete();

private:
    // 阶段
    enum class Phase {
        Entry, FreePlay, PostBloom, ShieldTutorial,
        WalkToPos, BulletIntro, BulletHell, DodgeIntro, DodgeTraining, Done
    };
    Phase m_phase = Phase::Entry;

    // 树摇曳
    struct TreeAnim { QPointer<QGraphicsTextItem> item; QAbstractAnimation *anim = nullptr; };
    QList<TreeAnim> m_treeAnims;
    QList<QPoint> m_treeBases;
    int m_bloomedCount = 0;
    bool m_bloomedFlags[3] = {false, false, false};

    void addTreeSway(QGraphicsTextItem *tree);
    void removeTreeSway(QGraphicsTextItem *target);

    // 转场黑屏
    void fadeToBlackAndBack(QGraphicsTextItem *player, QGraphicsTextItem *teacher,
                            QList<QGraphicsTextItem*> &waters,
                            std::function<void()> onDone);

    // 清除所有树和花
    void clearAllTreeItems(QList<QGraphicsTextItem*> &waters);

    // 自动行走
    void autoWalkPlayer(QGraphicsTextItem *player, QPoint target,
                        std::function<void()> onDone);

    // 弹幕系统
    QList<QGraphicsTextItem*> m_bullets;
    QTimer *m_bulletTimer = nullptr;
    qreal m_bulletX = 3.0;
    int m_bulletPhase = 0;
    bool m_bulletDodged = false;
    int m_fadeTick = 0;
    bool m_waitingForEnter = false;
    QGraphicsTextItem *m_enterHint = nullptr;
    QTimer *m_enterHintTimer = nullptr;

    void startBulletHell(QGraphicsTextItem *player, bool &inputEnabled);
    void tickBullets(QGraphicsTextItem *player);

    // 闪避训练
    struct FlyingBullet {
        QGraphicsTextItem *item = nullptr;
        qreal x = 0, y = 0;
        int dx = 0, dy = 0;
    };
    QList<FlyingBullet> m_flyingBullets;
    QGraphicsTextItem *m_teacherItem = nullptr;
    QTimer *m_dodgeTimer = nullptr;
    QTimer *m_teacherTimer = nullptr;
    QTimer *m_countdownTimer = nullptr;
    QGraphicsTextItem *m_hpDisplay = nullptr;
    QGraphicsTextItem *m_timerDisplay = nullptr;
    int m_hp = 3;
    int m_countdownSec = 20;
    int m_fireCooldown = 0;

    void startDodgeTraining(QGraphicsTextItem *player, QGraphicsTextItem *teacher,
                             QGraphicsTextItem *arrow, bool &inputEnabled);
    void tickDodge(QGraphicsTextItem *player, QGraphicsTextItem *arrow);
    void tickTeacher(QGraphicsTextItem *player);
    void fireBulletAtPlayer(QGraphicsTextItem *player);
    void checkDodgeCollision(QGraphicsTextItem *player);
    void endDodgeTraining(bool success);
    void updateHud();
    void clearFlyingBullets();
    void playEndingSequence(int idx = 0);
    void animateExitAndComplete();
    void shakeScreen();

    // 跨阶段持久引用
    QGraphicsTextItem *m_playerPtr = nullptr;
    QGraphicsTextItem *m_arrowPtr = nullptr;
    bool *m_inputPtr = nullptr;

    void onBulletHellFinished();
};

#endif
