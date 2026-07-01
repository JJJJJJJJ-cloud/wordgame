#ifndef LEVEL3ROAD_H
#define LEVEL3ROAD_H

#include "levelbase.h"

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QAbstractAnimation>
#include <QTimer>
#include <QList>
#include <QSet>
#include <QStringList>
#include <functional>

class DialogueSystem;

class Level3Road : public LevelBase
{
    Q_OBJECT
public:
    explicit Level3Road(QObject *parent = nullptr);

    // 初始化道路和车流
    void setup(QList<QGraphicsTextItem*> &pushables,
               QList<QGraphicsTextItem*> &waters,
               QList<QGraphicsTextItem*> &specialItems);

    // 道路阻挡：返回 true 表示移动被阻挡
    bool handleRoadBlock(QGraphicsTextItem *player);

    // 每步移动后检查胜利
    void checkWin(QGraphicsTextItem *player,
                  QList<QGraphicsTextItem*> &pushables,
                  QList<QGraphicsTextItem*> &waters,
                  QList<QGraphicsTextItem*> &specialItems,
                  bool &levelPassed, bool &inputEnabled,
                  std::function<void()> onFadeTransition);

    // 交互对话
    void advanceInteractiveDialogue(QList<QGraphicsTextItem*> &pushables,
                                     QList<QGraphicsTextItem*> &waters);
    bool hasInteractiveDialogue() const;
    void resetInteractiveDialogue();

    // 清理
    void cleanup() override;

signals:
    void warningShown();

private:
    QList<QGraphicsTextItem*> m_cars;
    QList<QAbstractAnimation*> m_carAnims;

    QGraphicsTextItem *m_warning = nullptr;
    QTimer *m_warningTimer = nullptr;
    bool m_roadBlocked = false;

    QList<QGraphicsTextItem*> m_interactiveChars;
    QSet<int> m_interactivePushableIndices;
    QStringList m_interactiveLines;
    int m_interactiveLineIdx = 0;

    void setupCars();
    void setupDialogue(QList<QGraphicsTextItem*> &pushables,
                       QList<QGraphicsTextItem*> &waters);
    void clearInteractiveLine(QList<QGraphicsTextItem*> *pushables = nullptr,
                               QList<QGraphicsTextItem*> *waters = nullptr);
    void showRoadWarning();
};

#endif
