#ifndef DEATHEFFECT_H
#define DEATHEFFECT_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QList>
#include <QStringList>
#include <functional>

class DialogueSystem;

class DeathEffect : public QObject
{
    Q_OBJECT
public:
    explicit DeathEffect(QObject *parent = nullptr);

    void setScene(QGraphicsScene *scene) { m_scene = scene; }
    void setDialogueSystem(DialogueSystem *ds) { m_dialogueSystem = ds; }

    bool isPlaying() const { return m_playing; }

    /// 启动死亡演出序列
    /// @param player          玩家 item
    /// @param deathDialogues  死亡对话（每行一句，逐句打字机播放）
    /// @param epilogueLines   结尾语（每行一句，位于遮罩之上）
    /// @param onDone          全部播完后回调
    /// @param epilogueColor   结尾语颜色（默认暗红）
    void play(QGraphicsTextItem *player,
              const QStringList &deathDialogues,
              const QStringList &epilogueLines,
              std::function<void()> onDone = nullptr,
              const QColor &epilogueColor = QColor(220, 40, 40));

    /// 强制清理所有临时元素（关卡切换时调用）
    void cleanup();

private:
    QGraphicsScene *m_scene = nullptr;
    DialogueSystem *m_dialogueSystem = nullptr;

    bool m_playing = false;
    QGraphicsTextItem *m_player = nullptr;
    QStringList m_deathDialogues;
    QStringList m_epilogueLines;
    QColor m_epilogueColor{Qt::white};
    std::function<void()> m_onDone;

    // —— 临时 item（统一管理，cleanup / 结束时清理） ——
    QList<QGraphicsTextItem*> m_tempItems;
    QGraphicsRectItem *m_overlay = nullptr;

    // —— 演出阶段 ——
    void animateFlash(int count = 0);
    void animateFall();
    void showDeathChars();
    void playDeathDialogue(int idx = 0);
    void showEpilogue(int idx = 0);
    void finish();
};

#endif
