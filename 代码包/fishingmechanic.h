#ifndef FISHINGMECHANIC_H
#define FISHINGMECHANIC_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QList>
#include <QStringList>
#include <functional>

class DialogueSystem;

class FishingMechanic : public QObject
{
    Q_OBJECT
public:
    explicit FishingMechanic(QObject *parent = nullptr);

    void setScene(QGraphicsScene *scene) { m_scene = scene; }
    void setDialogueSystem(DialogueSystem *ds) { m_dialogueSystem = ds; }

    // 初始化：收集湖底物品并启动随机移动
    void setup(QGraphicsTextItem *player,
               const QStringList &fishingItems,
               int fishingTargetIndex);

    // 放绳
    void dropRope(QGraphicsTextItem *player, bool inputEnabled, bool &inputEnabledRef);

    bool isActive() const { return m_fishingActive; }

    void retract();
    void finishRound();  // 结束本轮钓鱼：停绳、收绳、保留物品和移动

    // 关卡切换时完全清理
    void cleanup();

    // 第五关入场叙事：老师滑入 、 对话 、 走到钓鱼位
    void startLevel5Entry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                          bool &inputEnabled, bool &arrowEnabled,
                          const QStringList &fishingItems, int fishingTargetIndex);

signals:
    void catchComplete(bool isTarget);

private:
    QGraphicsScene *m_scene = nullptr;
    DialogueSystem *m_dialogueSystem = nullptr;

    bool m_fishingActive = false;
    QTimer *m_fishingTimer = nullptr;
    QTimer *m_moveTimer = nullptr;
    QList<QGraphicsTextItem*> m_ropeSegments;
    int m_ropeY = 0;

    QList<QGraphicsTextItem*> m_targets;
    QStringList m_fishingItemNames;
    int m_targetIndex = -1;

    QGraphicsTextItem *m_duck = nullptr;   // 水面上的鸭子（只左右移动）
    QTimer *m_duckTimer = nullptr;

    void tick(QGraphicsTextItem *player, bool &inputEnabledRef);
    void cleanupRope();
    void startRandomMovement(QGraphicsTextItem *player);
    void startDuckMovement();
};

#endif
