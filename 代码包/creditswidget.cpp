// 通关名单实现
#include "creditswidget.h"
#include <QPainter>
#include <QKeyEvent>

// 颜色 — 全部统一为金色
#define C_BG    QColor(0,0,0)
#define C_GOLD  QColor(255,215,0)

CreditsWidget::CreditsWidget(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(true);
    QPalette p = palette();
    p.setColor(QPalette::Window, Qt::black);
    setPalette(p);

    m_lines = {
        {"Main Menu", "Bouncy Platformer", "Eric Matyas"},
        {"Level 1", "Goings-On in Pixel Town", "Eric Matyas"},
        {"Level 2", "Coin-Op Strangeness", "Eric Matyas"},
        {"Level 3", "Arcade Oddities", "Eric Matyas"},
        {"Level 4", "Kingdom Quest", "Eric Matyas"},
        {"Level 5", "The Lost Island of Jewels", "Eric Matyas"},
        {"Level 6", "The City of Ice", "Eric Matyas"},
        {"Level 7", "End", "MusicWord"},
        {"Level 8", "Mystical", "leberch"},
        {"Level 9", "Dramatic Trailer", "BoDleasons"},
        {"Level 10", "Epic Fight", "NastelBom"},
        {"Level 11", "Sad", "leberch"},
        {"Level 12", "Ancient Empire", "The_Mountain"},
    };

    m_tick = new QTimer(this);
    connect(m_tick, &QTimer::timeout, this, &CreditsWidget::onTick);
}

void CreditsWidget::start()
{
    m_phase = FADE_IN;
    m_fadeAlpha = 0.f;
    m_scrollOffset = -height() * 0.08f;
    m_scrollDone = false;
    m_canExit = false;
    setFocus();
    m_tick->start(30);
    // 2 秒后即可按任意键退出
    QTimer::singleShot(2000, this, [this]() {
        m_canExit = true;
    });
}

void CreditsWidget::onTick()
{
    switch (m_phase) {
    case FADE_IN:
        m_fadeAlpha += 0.04f;
        if (m_fadeAlpha >= 1.f) {
            m_fadeAlpha = 1.f;
            m_phase = SCROLLING;
            beginScroll();
        }
        break;
    case SCROLLING: {
        m_scrollOffset += 1.45f;
        float lastY = height() - m_scrollOffset + (m_lines.size() - 1) * m_lineH;
        if (lastY < -20) {
            m_scrollDone = true;
            m_phase = FADING_TO_PROD;
            m_fadeAlpha = 0.f;
        }
        break;
    }
    case FADING_TO_PROD:
        m_fadeAlpha += 0.03f;
        if (m_fadeAlpha >= 1.f) {
            m_fadeAlpha = 1.f;
            m_phase = SHOWING_PROD;
            // 停留 4s 后等待按键
            QTimer::singleShot(4000, this, &CreditsWidget::endWait);
        }
        break;
    case SHOWING_PROD:
    case WAITING_KEY:
        break;
    }
    update();
}

void CreditsWidget::beginScroll() {}
void CreditsWidget::fadeToProducers() {}

void CreditsWidget::endWait()
{
    if (m_phase == SHOWING_PROD)
        m_phase = WAITING_KEY;
    update();
}

void CreditsWidget::keyPressEvent(QKeyEvent *)
{
    if (m_canExit) {
        m_tick->stop();
        emit finished();
    }
}

// 绘制
void CreditsWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    int w = width(), h = height();

    // 滚动阶段的列布局
    const float mL = w * 0.06f;
    const float uW = w - mL * 2;
    const float cTW = uW * 0.48f;
    const float side = (uW - cTW) / 2;
    const float cLX = mL, cLW = side;
    const float cTX = mL + cLW, cTW2 = cTW;
    const float cAX = mL + cLW + cTW, cAW = side;

    // 滚动名单
    if (m_phase == FADE_IN || m_phase == SCROLLING || m_scrollDone) {
        float startY = h - m_scrollOffset;

        // 标题
        float titleY = startY - 100;
        {
            QFont f(QString::fromUtf8("黑体"), 28, QFont::Bold);
            p.setFont(f);
            p.setPen(C_GOLD);
            p.drawText(QRectF(mL, titleY, uW, 36), Qt::AlignHCenter,
                       QString::fromUtf8("音  乐  制  作  人  名  单"));

            float lineY = titleY + 42;
            p.setPen(QPen(QColor(255, 215, 0, 90), 1));
            p.drawLine(cLX, lineY, cAX + cAW, lineY);

            QFont hf(QString::fromUtf8("黑体"), 17, QFont::Bold);
            p.setFont(hf);
            p.setPen(C_GOLD);
            float hy = lineY + 14;
            p.drawText(QRectF(cLX, hy, cLW, 28), Qt::AlignLeft | Qt::AlignVCenter,
                       QString::fromUtf8("关卡"));
            p.drawText(QRectF(cTX, hy, cTW2, 28), Qt::AlignCenter | Qt::AlignVCenter,
                       QString::fromUtf8("曲名"));
            p.drawText(QRectF(cAX, hy, cAW, 28), Qt::AlignRight | Qt::AlignVCenter,
                       QString::fromUtf8("作者"));
        }

        for (int i = 0; i < m_lines.size(); ++i) {
            float y = startY + i * m_lineH;
            if (y < -60 || y > h + 40) continue;
            const auto &ln = m_lines[i];

            if (i % 2 == 0)
                p.fillRect(QRectF(mL - 2, y + 2, uW + 4, m_lineH - 4),
                           QColor(255, 215, 0, 6));

            QFont lf(QString::fromUtf8("黑体"), 15, QFont::Bold);
            p.setFont(lf); p.setPen(C_GOLD);
            p.drawText(QRectF(cLX, y, cLW - 6, m_lineH),
                       Qt::AlignLeft | Qt::AlignVCenter, ln.level);

            QFont tf(QString::fromUtf8("黑体"), 17);
            p.setFont(tf); p.setPen(C_GOLD);
            p.drawText(QRectF(cTX, y, cTW2 - 6, m_lineH),
                       Qt::AlignCenter | Qt::AlignVCenter, ln.track);

            QFont af(QString::fromUtf8("黑体"), 15);
            p.setFont(af); p.setPen(QColor(255, 215, 0, 180));
            p.drawText(QRectF(cAX, y, cAW - 2, m_lineH),
                       Qt::AlignRight | Qt::AlignVCenter, ln.author);

            float sy = y + m_lineH - 2;
            p.setPen(QPen(QColor(255, 215, 0, 18), 1));
            p.drawLine(cLX + 8, sy, cAX + cAW - 8, sy);
        }

        // 遮罩
        QLinearGradient ft(0, 0, 0, 80), fb(0, h - 80, 0, h);
        ft.setColorAt(0, QColor(0, 0, 0)); ft.setColorAt(1, QColor(0, 0, 0, 0));
        fb.setColorAt(0, QColor(0, 0, 0, 0)); fb.setColorAt(1, QColor(0, 0, 0));
        p.fillRect(QRectF(0, 0, w, 80), ft);
        p.fillRect(QRectF(0, h - 80, w, 80), fb);
    }

    // 淡入遮罩
    if (m_phase == FADE_IN) {
        p.fillRect(rect(), QColor(0, 0, 0, 255 - int(m_fadeAlpha * 255)));
    }

    // 制作人层
    if (m_phase == FADING_TO_PROD || m_phase == SHOWING_PROD || m_phase == WAITING_KEY) {
        float a = (m_phase == WAITING_KEY) ? 1.f : m_fadeAlpha;
        p.setOpacity(a);

        p.fillRect(rect(), QColor(0, 0, 0, qMin(220, int(a * 220))));

        QFont tF(QString::fromUtf8("黑体"), 32, QFont::Bold);
        p.setFont(tF); p.setPen(C_GOLD);
        p.drawText(QRectF(0, h * 0.16f, w, 36), Qt::AlignHCenter,
                   QString::fromUtf8("制  作  人"));

        float lineY = h * 0.16f + 46;
        p.setPen(QPen(C_GOLD, 2));
        p.drawLine(w * 0.20f, lineY, w * 0.80f, lineY);

        const char *names[] = {u8"闫子桐", u8"张浩一", u8"张  翔"};
        float yOff[] = {0.35f, 0.50f, 0.65f};
        QFont nF(QString::fromUtf8("黑体"), 26, QFont::Bold);
        for (int i = 0; i < 3; ++i) {
            p.setFont(nF); p.setPen(C_GOLD);
            p.drawText(QRectF(0, h * yOff[i], w, 34), Qt::AlignHCenter,
                       QString::fromUtf8(names[i]));
        }

        QFont fnF(QString::fromUtf8("黑体"), 11);
        p.setFont(fnF); p.setPen(QColor(255, 215, 0, 120));
        p.drawText(QRectF(0, h * 0.80f, w, 20), Qt::AlignHCenter,
                   "Powered by Pixabay & Eric Matyas");

        p.setOpacity(1.0);
    }

    // 按任意键提示（2秒后始终显示）
    if (m_canExit) {
        QFont kF(QString::fromUtf8("黑体"), 16);
        p.setFont(kF); p.setPen(C_GOLD);
        p.drawText(QRectF(0, h * 0.92f, w, 28), Qt::AlignHCenter,
                   QString::fromUtf8("— 按任意键返回主菜单 —"));
    }
}
