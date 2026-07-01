#ifndef PAUSEMENU_H
#define PAUSEMENU_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QList>
#include <QString>
#include <QColor>

class PauseMenu : public QObject
{
    Q_OBJECT
public:
    explicit PauseMenu(QObject *parent = nullptr);

    void setScene(QGraphicsScene *scene) { m_scene = scene; }
    void setCurrentLevel(int level) { m_currentLevel = level; }

    bool isActive() const { return m_active; }
    void open(QGraphicsTextItem *playerItem);
    void close();
    void handleKeyPress(int key);

signals:
    void resumeGame();
    void resetLevelRequested();
    void returnToMenuRequested();
    void loadLevelRequested(int level);

private:
    QGraphicsScene *m_scene = nullptr;
    // 保留游戏场景中玩家的引用，用于 open/close 的位置恢复
    QGraphicsTextItem *m_gamePlayer = nullptr;

    bool m_active = false;
    bool m_isPostGame = false;
    enum class State { Inactive, Main, ConfirmReturn, ConfirmMenu, ShowHint, MemorySelect };
    State m_state = State::Inactive;

    //调色板
    QColor m_palTitle;       // 标题 / 选中文字
    QColor m_palText;        // 普通文字
    QColor m_palDim;         // 未解锁 / 灰字
    QColor m_palBorder;      // 面板边框
    QColor m_palBg;          // 面板背景
    QColor m_palAccent;      // 强调 / 已通关标记
    void initPalette();

    //图形项
    QGraphicsRectItem *m_overlay = nullptr;
    QList<QGraphicsTextItem*> m_items;
    QList<QGraphicsRectItem*> m_panels;
    QGraphicsTextItem *m_title = nullptr;
    QGraphicsTextItem *m_confirmText = nullptr;
    QGraphicsTextItem *m_hintText = nullptr;
    QGraphicsTextItem *m_subtitle = nullptr;
    QGraphicsTextItem *m_cursor = nullptr;   // 独立的"我"光标，不和场景玩家共用
    QTimer *m_subtitleTimer = nullptr;

    int m_selection = 0;
    int m_optionCount = 0;
    int m_currentLevel = 1;
    QString m_pendingAction;

    void buildMain(int sel = -1);
    void buildConfirm(const QString &msg, int sel = -1);
    void buildMemorySelect(int sel = -1);
    void clearAll();
    void navigate(int dx, int dy);
    void confirm();
    void updateSubtitle();
    void placeCursorAtSelection();  // 将"我"摆到当前选项旁

    //绘制辅助
    QGraphicsRectItem *addPanel(qreal x, qreal y, qreal w, qreal h,
                                const QColor &fill, const QColor &border,
                                qreal z = 999);
    void drawScanlineOverlay();
};

#endif
