// 通关名单控件 — 淡入 、 滚动音乐 、 制作人 、 按任意键回主菜单
#ifndef CREDITSWIDGET_H
#define CREDITSWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QVector>

struct CreditLine {
    QString level;
    QString track;
    QString author;
};

class CreditsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CreditsWidget(QWidget *parent = nullptr);

    void start();  // 开始播放（带淡入）

signals:
    void finished();  // 全部播完 + 用户按键

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum Phase { FADE_IN, SCROLLING, FADING_TO_PROD, SHOWING_PROD, WAITING_KEY };
    Phase m_phase = FADE_IN;
    float m_fadeAlpha = 0.f;       // 淡入/制作人透明度
    float m_scrollOffset = 0.f;    // 滚动偏移
    int m_lineH = 62;
    bool m_scrollDone = false;
    bool m_canExit = false;
    QVector<CreditLine> m_lines;

    QTimer *m_tick;
    void onTick();
    void beginScroll();
    void fadeToProducers();
    void endWait();
};

#endif
