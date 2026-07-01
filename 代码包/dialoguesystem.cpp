#include "dialoguesystem.h"
#include "typewriter.h"
#include "grid.h"

#include <QFont>
#include <QTimer>

DialogueSystem::DialogueSystem(QObject *parent)
    : QObject(parent)
{
    m_typewriter = new TypewriterEffect(this);
}

// 状态查询
bool DialogueSystem::isActive() const {
    return m_state != State::Inactive;
}

// 独立对话
void DialogueSystem::display(const QString &text, int gx, int gy, int fontSize,
                              std::function<void()> onDone) {
    cleanup();
    if (onDone) m_onDone = std::move(onDone);
    m_item = m_scene->addText("", QFont("黑体", fontSize));
    m_item->setDefaultTextColor(m_displayColor);
    m_item->setPos(Grid::toPixel(gx, gy));
    m_item->setZValue(m_displayZValue);
    m_state = State::Typing;
    m_typewriter->start(m_item, text, 80);
    connect(m_typewriter, &TypewriterEffect::finished, this, [this]() {
        m_state = State::Waiting;
        showCursor();
    }, Qt::SingleShotConnection);
}

// 对话序列位置设置
void DialogueSystem::setSequencePos(int gx, int gy, int fontSize) {
    m_seqGx = gx;
    m_seqGy = gy;
    m_seqFontSize = fontSize;
}

// 对话序列
void DialogueSystem::showSequence(const QStringList &lines, std::function<void()> onDone) {
    m_dialogues = lines;
    m_dialogueIndex = 0;
    m_onDone = std::move(onDone);
    showNext();
}

void DialogueSystem::showNext() {
    if (m_dialogueIndex >= m_dialogues.size()) {
        cleanup();
        auto cb = m_onDone;
        m_onDone = nullptr;
        if (cb) cb();
        emit sequenceFinished();
        return;
    }
    display(m_dialogues[m_dialogueIndex++], m_seqGx, m_seqGy, m_seqFontSize);
}

// 推进对话
void DialogueSystem::advance() {
    if (m_state == State::Inactive) return;

    if (m_state == State::Typing) {
        // 打字中 、 立即显示全文
        hideCursor();
        m_typewriter->skip();
        m_state = State::Waiting;
        showCursor();
        return;
    }

    if (m_state == State::Waiting) {
        // L3 交互式对话
        if (m_interactiveAdvanceCb) {
            hideCursor();
            m_interactiveAdvanceCb();
            return;
        }
        // 全文显示中 、 下一句或关闭
        hideCursor();
        if (m_dialogues.isEmpty()) {
            // 单独的对话 、 关闭
            cleanup();
            auto cb = m_onDone;
            m_onDone = nullptr;
            if (cb) cb();
        } else {
            // 对话序列 、 下一句
            cleanup();
            showNext();
        }
    }
}

// 清理
void DialogueSystem::cleanup() {
    hideCursor();
    m_state = State::Inactive;
    if (m_item) {
        m_scene->removeItem(m_item);
        delete m_item;
        m_item = nullptr;
    }
    m_typewriter->disconnect();
}

// 交互字符（L1/L3 用）
QList<QGraphicsTextItem*> DialogueSystem::placeInteractiveText(
    const QString &text, int startGx, int gy,
    const QSet<int> & /*pushableIndices*/) {
    QList<QGraphicsTextItem*> result;
    QFont f("黑体", 22);
    for (int i = 0; i < text.size(); ++i) {
        QGraphicsTextItem *ch = m_scene->addText(QString(text[i]), f);
        ch->setDefaultTextColor(Qt::white);
        ch->setPos(Grid::toPixel(startGx + i, gy));
        result.append(ch);
    }
    return result;
}

// 对话光标（跳动三角标）
void DialogueSystem::showCursor() {
    if (!m_scene || !m_item) return;

    if (!m_cursor) {
        m_cursor = m_scene->addText("▼", QFont("黑体", 10));
        m_cursor->setDefaultTextColor(Qt::white);
        m_cursor->setZValue(m_displayZValue + 1);
    }
    m_cursor->setVisible(true);
    m_cursorTick = 0;
    updateCursorPos();

    if (!m_cursorTimer) {
        m_cursorTimer = new QTimer(this);
        connect(m_cursorTimer, &QTimer::timeout, this, [this]() {
            if (!m_cursor || !m_cursor->isVisible()) return;
            m_cursorTick++;
            // 上下跳动：幅度 ±3px，频率约 6Hz
            qreal offset = 3.0 * std::sin(m_cursorTick * 0.35);
            // 基准Y在 updateCursorPos 中设置，这里通过 data(0) 存取
            qreal baseY = m_cursor->data(0).toDouble();
            m_cursor->setY(baseY + offset);
        });
    }
    if (!m_cursorTimer->isActive())
        m_cursorTimer->start(40);
}

void DialogueSystem::updateCursorPos() {
    if (!m_cursor || !m_item) return;
    QRectF br = m_item->boundingRect();
    QPointF itemPos = m_item->pos();
    // 放在文本末尾右侧偏下
    qreal cx = itemPos.x() + br.width() + 4;
    qreal cy = itemPos.y() + br.height() - 6;
    m_cursor->setPos(cx, cy);
    m_cursor->setData(0, cy);  // 存储基准Y供跳动使用
}

void DialogueSystem::hideCursor() {
    if (m_cursorTimer) {
        m_cursorTimer->stop();
    }
    if (m_cursor) {
        if (m_cursor->scene())
            m_cursor->scene()->removeItem(m_cursor);
        delete m_cursor;
        m_cursor = nullptr;
    }
}

void DialogueSystem::setDisplayZValue(qreal z) {
    m_displayZValue = z;
    if (m_item) m_item->setZValue(z);
    if (m_cursor) m_cursor->setZValue(z + 1);
}

void DialogueSystem::setDisplayColor(const QColor &color) {
    m_displayColor = color;
    if (m_item) m_item->setDefaultTextColor(color);
}
