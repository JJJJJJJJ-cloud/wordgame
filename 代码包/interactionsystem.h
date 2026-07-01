#ifndef INTERACTIONSYSTEM_H
#define INTERACTIONSYSTEM_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QList>
#include <QSet>
#include <QPoint>
#include <functional>

class DialogueSystem;

// 一行交互对话的配置
struct InteractionLine {
    QString text;
    QSet<int> pushableIndices;
    bool smallFont = false;  // 代码等长文本用小字号
};

// 一个交互点的完整配置
struct Interaction {
    QPoint gridPos;
    QList<InteractionLine> lines;
};

class InteractionSystem : public QObject
{
    Q_OBJECT
public:
    explicit InteractionSystem(QObject *parent = nullptr);

    void setScene(QGraphicsScene *scene) { m_scene = scene; }
    void setDialogueSystem(DialogueSystem *ds) { m_dialogueSystem = ds; }

    // 注册交互
    void registerInteraction(const Interaction &inter);

    // F 键触发：检查箭头前方是否有可交互物
    bool tryInteract(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                     QList<QGraphicsTextItem*> &pushables,
                     QList<QGraphicsTextItem*> &waters,
                     bool &inputEnabled);

    // 推进对话（下一句）
    void advance(QList<QGraphicsTextItem*> &pushables,
                 QList<QGraphicsTextItem*> &waters,
                 bool &inputEnabled);

    bool isActive() const { return m_active; }

    // 当前交互的格子位置（供关卡逻辑使用）
    QPoint currentGridPos() const { return m_currentGrid; }

    // 当前对话行完整文本
    QString currentLineText() const { return m_currentFullText; }

    // 检查当前可见字符拼成的文字
    QString currentVisibleText() const;

    void cleanup(QList<QGraphicsTextItem*> &pushables,
                 QList<QGraphicsTextItem*> &waters);

    // 通知某个 item 已被外部删除（防止野指针）
    void notifyItemDeleted(QGraphicsTextItem *item);

signals:
    void interactionStarted(QPoint gridPos);
    void interactionFinished(QPoint gridPos);

private:
    QGraphicsScene *m_scene = nullptr;
    DialogueSystem *m_dialogueSystem = nullptr;

    QList<Interaction> m_interactions;
    Interaction *findInteraction(QPoint gridPos);
    bool m_active = false;
    QPoint m_currentGrid;
    int m_currentLineIdx = 0;
    QString m_currentFullText;

    QList<QGraphicsTextItem*> m_currentChars;
    QGraphicsTextItem *m_cursorItem = nullptr;
    QTimer *m_revealTimer = nullptr;
    int m_revealIndex = 0;

    void revealStep();
    void skipReveal();

    void showLine(const InteractionLine &line, int startX, int gy,
                  QList<QGraphicsTextItem*> &pushables,
                  QList<QGraphicsTextItem*> &waters);
    void clearCurrentLine(QList<QGraphicsTextItem*> &pushables,
                          QList<QGraphicsTextItem*> &waters);
};

#endif
