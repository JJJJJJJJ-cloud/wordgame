#ifndef DIALOGUESYSTEM_H
#define DIALOGUESYSTEM_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QStringList>
#include <QSet>
#include <functional>

class TypewriterEffect;

class DialogueSystem : public QObject
{
    Q_OBJECT
public:
    explicit DialogueSystem(QObject *parent = nullptr);

    void setScene(QGraphicsScene *scene) { m_scene = scene; }

    // --- 公有 API ---
    bool isActive() const;
    void advance();
    void display(const QString &text, int gx, int gy, int fontSize = 18,
                 std::function<void()> onDone = nullptr);
    void showSequence(const QStringList &lines, std::function<void()> onDone = nullptr);
    void setSequencePos(int gx, int gy, int fontSize = 18);
    void cleanup();

    // 放置可交互字符行（L1/L3 用），返回所有被添加的字符项
    QList<QGraphicsTextItem*> placeInteractiveText(
        const QString &text, int startGx, int gy,
        const QSet<int> &pushableIndices);

    // L3 交互式对话钩子
    void setInteractiveAdvanceCallback(std::function<void()> cb) {
        m_interactiveAdvanceCb = std::move(cb);
    }
    void clearInteractiveAdvanceCallback() { m_interactiveAdvanceCb = nullptr; }

    // L3 用：强制进入 Waiting 状态（不在打字，直接等 E 键触发回调）
    void activateWaitingState() { m_state = State::Waiting; }

    // L3 交互对话状态查询
    bool hasInteractiveLines() const { return !m_interactiveLines.isEmpty(); }
    void setInteractiveLines(const QStringList &lines) { m_interactiveLines = lines; }
    void clearInteractiveLines() { m_interactiveLines.clear(); }
    int interactiveLineIndex() const { return m_interactiveLineIdx; }
    void setInteractiveLineIndex(int idx) { m_interactiveLineIdx = idx; }

    TypewriterEffect *typewriter() const { return m_typewriter; }

    // 设置对话文字和光标的 zValue（用于分层渲染，如死亡遮罩）
    void setDisplayZValue(qreal z);
    // 设置对话文字颜色（如死亡结尾语用红色）
    void setDisplayColor(const QColor &color);

signals:
    void sequenceFinished();

private:
    QGraphicsScene *m_scene = nullptr;
    TypewriterEffect *m_typewriter = nullptr;

    enum class State { Inactive, Typing, Waiting };
    State m_state = State::Inactive;

    QGraphicsTextItem *m_item = nullptr;
    QStringList m_dialogues;
    int m_dialogueIndex = 0;
    std::function<void()> m_onDone;

    // L3 交互式对话钩子 + 状态
    std::function<void()> m_interactiveAdvanceCb;
    QStringList m_interactiveLines;
    int m_interactiveLineIdx = 0;

    QGraphicsTextItem *m_cursor = nullptr;
    QTimer *m_cursorTimer = nullptr;
    int m_cursorTick = 0;
    qreal m_displayZValue = 0;   // 对话文字 / 光标的 zValue
    QColor m_displayColor{Qt::white};  // 对话文字颜色

    // showSequence 使用的显示位置
    int m_seqGx = 2;
    int m_seqGy = 12;
    int m_seqFontSize = 18;

    void showCursor();
    void hideCursor();
    void updateCursorPos();

    void showNext();
};

#endif
