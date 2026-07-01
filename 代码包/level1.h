#ifndef LEVEL1_H
#define LEVEL1_H

#include "levelbase.h"

#include <QGraphicsTextItem>
#include <QSet>
#include <functional>

class DialogueSystem;

/// 第一关：宿舍醒来 、 推"没"字开门
class Level1 : public LevelBase
{
    Q_OBJECT
public:
    explicit Level1(QObject *parent = nullptr);

    /// 入场叙事：对话 + 文字揭示
    /// revealFn 由 GameScene 注入，用于在对话结束后触发文字逐个揭示
    void startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                    bool &inputEnabled, bool &arrowEnabled,
                    std::function<void(const QString &text, int startGx, int gy,
                                       const QSet<int> &pushable,
                                       std::function<void()> onDone)> revealFn);
};

#endif // LEVEL1_H
