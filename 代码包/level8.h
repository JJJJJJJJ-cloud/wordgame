#ifndef LEVEL8_H
#define LEVEL8_H

#include "levelbase.h"

#include <QAbstractAnimation>
#include <QList>
#include <QPoint>
#include <QPointer>
#include <QTimer>

class DialogueSystem;
class InteractionSystem;

class Level8 : public LevelBase
{
    Q_OBJECT
public:
    explicit Level8(QObject *parent = nullptr);

    void setInteractionSystem(InteractionSystem *is) { m_interactionSystem = is; }

    // 入口序列
    void startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                    bool &inputEnabled);

    // 注册所有 F 键交互
    void registerInteractions();

    // 检查通关条件（石头交互中删除了"不"）
    bool tryStoneBreak(QList<QGraphicsTextItem*> &pushables,
                       QList<QGraphicsTextItem*> &waters,
                       bool &inputEnabled,
                       QGraphicsTextItem *player);

    void cleanup() override;

signals:
    void entryComplete();
    void stonesBroken();
    void level8Complete();

private:
    InteractionSystem *m_interactionSystem = nullptr;

    // 环境动画
    QList<QAbstractAnimation*> m_ambientAnims;
    QList<QGraphicsTextItem*> m_ambientItems;
    QList<QTimer*> m_ambientTimers;

    void startSparks(QGraphicsTextItem *computer);
    void startShiver(QGraphicsTextItem *person);
    void startGroan(QGraphicsTextItem *person);

    // 石头列表
    QList<QGraphicsTextItem*> m_stones;
    bool m_stonesBroken = false;
    QGraphicsTextItem *m_doorItem = nullptr;
};

#endif
