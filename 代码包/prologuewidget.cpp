#include "prologuewidget.h"
#include <QPainter>
#include <QFont>
#include <QPalette>

PrologueWidget::PrologueWidget(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);

    m_typeTimer = new QTimer(this);
    m_typeTimer->setInterval(80);
    connect(m_typeTimer, &QTimer::timeout, this, [this]() {
        if (m_currentLine >= m_lines.size()) {
            m_typeTimer->stop();
            m_active = false;
            m_cursorVisible = true;
            update();
            m_finishTimer->start(2000);
            return;
        }
        const QString &curLine = m_lines[m_currentLine];
        if (m_currentCol >= curLine.length()) {
            m_currentLine++;
            m_currentCol = 0;
            if (m_currentLine >= m_lines.size()) {
                m_typeTimer->stop();
                m_active = false;
                m_cursorVisible = true;
                update();
                m_finishTimer->start(2000);
                return;
            }
            update();
            return;
        }
        m_currentCol++;
        update();
    });

    m_cursorTimer = new QTimer(this);
    m_cursorTimer->setInterval(530);
    connect(m_cursorTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;
        update();
    });

    m_finishTimer = new QTimer(this);
    m_finishTimer->setSingleShot(true);
    connect(m_finishTimer, &QTimer::timeout, this, [this]() {
        m_cursorTimer->stop();
        m_cursorVisible = false;
        update();
        emit finished();
    });
}

void PrologueWidget::start()
{
    const QString fullText = QStringLiteral(
        "我是一名普通的贵校学生。\n"
        "正在为程设的大作业发愁。\n"
        "没有任何灵感。没有任何灵感。没有任何灵感。\n"
        "眼看DDL临近，我愈发焦虑。\n"
        "从床上惊醒，不得已迎来新的一天。\n"
    );
    m_lines = fullText.split('\n', Qt::SkipEmptyParts);
    m_currentLine = 0;
    m_currentCol = 0;
    m_active = true;
    m_cursorVisible = true;

    m_cursorTimer->start();
    m_typeTimer->start();
    update();
}

void PrologueWidget::stop()
{
    m_typeTimer->stop();
    m_cursorTimer->stop();
    m_finishTimer->stop();
    m_active = false;
    m_cursorVisible = false;
    update();
}

void PrologueWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), Qt::black);

    QFont font("SimHei", kFontSize);
    p.setFont(font);
    p.setPen(m_isPostGame ? QColor("#ffd700") : QColor("#00ff88"));

    // 已打完的行（完整绘制）
    for (int i = 0; i < m_currentLine; i++) {
        int px = kStartX * kGridSize;
        int py = (kStartY + i) * kGridSize + kFontSize;
        p.drawText(px, py, m_lines[i]);
    }

    // 当前行（部分绘制）
    if (m_currentLine < m_lines.size() && m_currentCol > 0) {
        int px = kStartX * kGridSize;
        int py = (kStartY + m_currentLine) * kGridSize + kFontSize;
        p.drawText(px, py, m_lines[m_currentLine].left(m_currentCol));
    }

    // 闪烁光标
    if (m_cursorVisible && m_active) {
        int cursorX = (kStartX + m_currentCol) * kGridSize;
        int cursorY = (kStartY + m_currentLine) * kGridSize;
        QFont cursorFont("SimHei", kFontSize);
        p.setFont(cursorFont);
        p.setPen(m_isPostGame ? QColor("#ffd700") : QColor("#00ff88"));
        p.drawText(cursorX, cursorY + kFontSize, "▌");
    }
}
