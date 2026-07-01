#include "typewriter.h"

TypewriterEffect::TypewriterEffect(QObject *parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (!m_item) {
            m_timer->stop();
            m_running = false;
            return;
        }
        m_index++;
        m_item->setPlainText(m_fullText.left(m_index));
        if (m_index >= m_fullText.length()) {
            m_timer->stop();
            m_running = false;
            emit finished();
        }
    });
}

void TypewriterEffect::start(QGraphicsTextItem *item, const QString &fullText, int intervalMs)
{
    if (m_running) {
        m_timer->stop();
        m_running = false;
    }
    m_item = item;
    m_fullText = fullText;
    m_index = 0;
    m_running = true;
    if (m_item)
        m_item->setPlainText("");
    m_timer->start(intervalMs);
}

void TypewriterEffect::skip()
{
    if (!m_running || !m_item) return;
    m_timer->stop();
    m_item->setPlainText(m_fullText);
    m_index = m_fullText.length();
    m_running = false;
    emit finished();
}
