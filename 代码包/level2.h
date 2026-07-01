#ifndef LEVEL2_H
#define LEVEL2_H

#include "levelbase.h"

#include <QGraphicsTextItem>
#include <QSet>
#include <functional>

class DialogueSystem;

/// 第二关：食堂排队 、 拼"早上八点"拿到鸡汤面
class Level2 : public LevelBase
{
    Q_OBJECT
public:
    explicit Level2(QObject *parent = nullptr);

    /// 入场叙事：对话 + 文字揭示 + 排队抱怨气泡
    void startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                    bool &inputEnabled, bool &arrowEnabled,
                    std::function<void(const QString &text, int startGx, int gy,
                                       const QSet<int> &pushable,
                                       std::function<void()> onDone)> revealFn);

    void cleanup() override;

private:
    void setupQueueBubbles();
    void showBubble(const QString &text, int gx, int gy, int stayMs, QColor color);
};

#endif // LEVEL2_H
