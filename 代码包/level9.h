#ifndef LEVEL9_H
#define LEVEL9_H

#include "levelbase.h"

#include <QAbstractAnimation>
#include <QList>
#include <QPoint>
#include <QPointer>
#include <QTimer>

class DialogueSystem;
class DeleteEffect;
class DeathEffect;

class Level9 : public LevelBase
{
    Q_OBJECT
public:
    explicit Level9(QObject *parent = nullptr);

    // 设置物品列表指针（由 GameScene 在 loadLevel 时调用）
    void setItemLists(QList<QGraphicsTextItem*> &pushables,
                      QList<QGraphicsTextItem*> &waters) {
        m_pushablesPtr = &pushables;
        m_watersPtr = &waters;
    }

    // 入场序列
    void startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                    bool &inputEnabled);

    // Enter 键：弹反弹幕
    void handleEnterPress();

    // Backspace 键：删除红色文字中的关键字
    bool handleDeleteForward(QList<QGraphicsTextItem*> &pushables,
                             QList<QGraphicsTextItem*> &waters,
                             bool &inputEnabled,
                             QGraphicsTextItem *player);

    // 查询：某个 item 是否是当前关卡的可删除目标
    bool isDeletableTarget(QGraphicsTextItem *item) const;

    void cleanup() override;

signals:
    void entryComplete();
    void level9Complete();

private:
    // 阶段管理
    enum class Phase {
        Entry, WalkTo13, Dialogue1, WalkTo12, Dialogue2,
        Shake, Dialogue3, WalkTo11, Dialogue4, FreePlay,
        BulletHell, Dialogue5, WalkBackTo13, Dialogue6,
        RedText1, AutoWalkDelete1, RedText2, AutoWalkDelete2,
        FinalDialogue, WhiteOut, Done, Death
    };
    Phase m_phase = Phase::Entry;

    // 持久引用
    QGraphicsTextItem *m_player = nullptr;
    QGraphicsTextItem *m_arrow = nullptr;
    bool *m_inputEnabledPtr = nullptr;
    QList<QGraphicsTextItem*> *m_pushablesPtr = nullptr;
    QList<QGraphicsTextItem*> *m_watersPtr = nullptr;

    // 恶龙环境
    QList<QGraphicsTextItem*> m_dragonChars;
    QList<QAbstractAnimation*> m_dragonAnims;
    void setupDragonAmbient();

    // 弹幕系统
    QList<QGraphicsTextItem*> m_bullets;
    QTimer *m_bulletTimer = nullptr;
    int m_bulletFadeTick = 0;
    qreal m_bulletY = 8.0;
    int m_bulletPhase = 0;
    bool m_bulletParried = false;

    void startBulletHell();
    void tickBullets();
    void checkBulletPlayerCollision();
    void onBulletParried();
    void onBulletAllPassed();

    // 红色文字
    QList<QGraphicsTextItem*> m_redText1Chars;
    QList<QGraphicsTextItem*> m_redText2Chars;
    QGraphicsTextItem *m_deleteTarget1 = nullptr;
    QGraphicsTextItem *m_deleteTarget2 = nullptr;
    bool m_redText1Cleared = false;
    bool m_redText2Cleared = false;

    void spawnRedText1(QList<QGraphicsTextItem*> &waters);
    void spawnRedText2(QList<QGraphicsTextItem*> &waters);
    void clearRedText1Collision(QList<QGraphicsTextItem*> &waters);
    void clearRedText2Collision(QList<QGraphicsTextItem*> &waters);

    // 自动行走（重写基类 autoWalkStep）
    void autoWalkStep() override;

    // 死亡系统
    bool m_hasDied = false;
    void triggerDeath();

    // 屏幕震动
    void shakeScreen();

    // 白屏过渡
    void startWhiteOut();
};

#endif
