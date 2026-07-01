#ifndef DELETEEFFECT_H
#define DELETEEFFECT_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <functional>

// 通用删除动画：在 source 位置显示放大渐隐的 "←"，
// 同时 target 渐隐消失。可用于任意关卡的 backspace 删除操作。
class DeleteEffect : public QObject
{
    Q_OBJECT
public:
    explicit DeleteEffect(QObject *parent = nullptr);

    void setScene(QGraphicsScene *scene) { m_scene = scene; }
    bool isPlaying() const { return m_playing; }

    // 播放删除动画（← 特效），完成后回调 onDone 并清理临时元素
    void play(QGraphicsTextItem *source, QGraphicsTextItem *target,
              std::function<void()> onDone = nullptr);

    // 播放Enter之盾特效（↵ 渐显放大再渐隐）
    void playEnterShield(QGraphicsTextItem *player);

private:
    QGraphicsScene *m_scene = nullptr;
    bool m_playing = false;
};

#endif
