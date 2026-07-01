#ifndef LEVEL4CLASSROOM_H
#define LEVEL4CLASSROOM_H

#include "levelbase.h"

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QList>
#include <functional>

class DialogueSystem;

class Level4Classroom : public LevelBase
{
    Q_OBJECT
public:
    explicit Level4Classroom(QObject *parent = nullptr);

    int phase() const { return m_phase; }
    void setPhase(int p) { m_phase = p; }

    // 入场序列
    void startEntry(QGraphicsTextItem *player, bool &inputEnabled);

    // 检查玩家是否到达触发位置
    bool checkProgressTrigger(QGraphicsTextItem *player,
                              const QPoint &triggerGrid,
                              bool &inputEnabled);

    // 叙事序列（由 GameScene 触发）
    void doNarrative(std::function<void()> onDone);
    void doTeacherWalk(QList<QGraphicsTextItem*> &waters,
                       QGraphicsTextItem *player,
                       std::function<void()> onDone);

    //教室布景 + 动画
    // 生成三排"人"字碰撞布景，加入 waters 列表
    void setupClassroom(QList<QGraphicsTextItem*> &waters);

    // 绊线检测：若玩家在 (gx=16..19, gy=4)，触发绊倒事件
    void checkTripWire(const QPoint &playerGrid, QGraphicsTextItem *player,
                       QList<QGraphicsTextItem*> &waters, bool &inputEnabled);

    // 清理所有动画定时器和临时物品
    void cleanup() override;

signals:
    void entryComplete();
    void narrativeComplete();

private:
    int m_phase = 0;

    //ZZZ 动画
    QGraphicsTextItem *m_zzzPerson = nullptr;
    QList<QGraphicsTextItem*> m_zzzItems;
    QTimer *m_zzzTimer = nullptr;
    int m_zzzPhase = 0;   // 0=等第一个, 1=等第二个, 2=等第三个, 3=等消失
    void zzzTick();

    //绊线
    bool m_wireTriggered = false;
    QList<QGraphicsTextItem*> m_wireItems;
    void triggerTripWire(QGraphicsTextItem *player,
                         QList<QGraphicsTextItem*> &waters, bool &inputEnabled);

    //打字"嗒"
    QList<QGraphicsTextItem*> m_typeChars;
    QTimer *m_typeTimer = nullptr;
    QTimer *m_bounceTimer = nullptr;
    int m_typeSpawnIndex = 0;
    int m_bounceIndex = 0;
    void typeTick(QList<QGraphicsTextItem*> &waters);
    void bounceTick();
};

#endif
