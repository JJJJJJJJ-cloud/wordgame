#ifndef LEVEL11_H
#define LEVEL11_H

#include "levelbase.h"

#include <QGraphicsLineItem>
#include <QList>
#include <QMap>
#include <QSet>
#include <QPoint>
#include <QTimer>

class DialogueSystem;

//  Level11 — 未名湖畔 · 终幕交心
class Level11 : public LevelBase
{
    Q_OBJECT
public:
    explicit Level11(QObject *parent = nullptr);

    // 设置物品列表指针
    void setItemLists(QList<QGraphicsTextItem*> &pushables,
                      QList<QGraphicsTextItem*> &waters) {
        Q_UNUSED(pushables);
        m_watersPtr = &waters;
    }

    // 入场序列："我"从左上角飘到 (10,11) 、 交心对话 、 ... 、 黑屏
    void startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                    bool &inputEnabled);

    void cleanup();

signals:
    void level11Complete();

private:
    QList<QGraphicsTextItem*> *m_watersPtr = nullptr;

    // 阶段
    enum class Phase {
        Entry,         // "我"飘向 (10,11)
        Dialogue1,     // 交心对话
        DragonToAC,    // 龙 、 AC 逐排蜕变
        SunToXi,       // 日沉为夕
        Dialogue2,     // 夕阳叙事
        XiGlow,        // 夕辉光芒
        TreesToDream,  // 林+夕、梦 波浪融合
        FadeOut,       // 黑屏结束
        Done
    };
    Phase m_phase = Phase::Entry;

    // 核心引用
    QGraphicsTextItem *m_player = nullptr;
    QGraphicsTextItem *m_arrow = nullptr;
    bool *m_inputEnabledPtr = nullptr;

    // 龙字符（按 y 分组）
    QMap<int, QList<QGraphicsTextItem*>> m_dragonByY;
    QList<int> m_dragonYValues;  // 排序后的 y 值列表

    // "林"字符
    QList<QGraphicsTextItem*> m_trees;

    // "日"与"夕"
    QGraphicsTextItem *m_sun = nullptr;    // (2,2) 的"日"
    QGraphicsTextItem *m_xi = nullptr;     // "日"下移后在 (2,2) 出现的"夕"

    // 临时元素（统一管理，cleanup 时清理）
    QList<QGraphicsTextItem*> m_tempItems;       // AC / 梦 / 树下的夕 等
    QList<QGraphicsLineItem*> m_rays;            // 夕辉光芒

    // 内部方法

    //  收集
    void collectDragonChars();
    void collectTreeChars();

    //  阶段驱动
    void startDialogue1();
    void startDragonToAC();
    void animateDragonRow(int rowIndex);
    void startSunToXi();
    void startDialogue2();
    void startXiGlow();
    void fadeOutRays();
    void startTreesToDream();
    void animateTreeWave(int waveIndex);
    void finalFadeOut();

    // 工具
    QGraphicsTextItem *addTempText(const QString &text, int gx, int gy,
                                   int fontSize, QColor color,
                                   qreal opacity = 1.0, qreal z = 10);
};

#endif
