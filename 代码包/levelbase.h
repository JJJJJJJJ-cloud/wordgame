#ifndef LEVELBASE_H
#define LEVELBASE_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QPoint>
#include <QTimer>
#include <functional>

class DialogueSystem;
class DeleteEffect;
class DeathEffect;

/// 所有关卡类的公共基类，提供各关卡共用的场景/视图依赖、
/// 对话辅助与网格工具方法。
class LevelBase : public QObject
{
    Q_OBJECT
public:
    explicit LevelBase(QObject *parent = nullptr) : QObject(parent) {}

    // —— 依赖注入 ——
    void setScene(QGraphicsScene *scene) { m_scene = scene; }
    void setView(QGraphicsView *view) { m_view = view; }
    void setDialogueSystem(DialogueSystem *ds) { m_dialogueSystem = ds; }
    void setDeleteEffect(DeleteEffect *de) { m_deleteEffect = de; }
    void setDeathEffect(DeathEffect *de) { m_deathEffect = de; }

    virtual void cleanup();

protected:
    // —— 公共字段 ——
    QGraphicsScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    DialogueSystem *m_dialogueSystem = nullptr;
    DeleteEffect *m_deleteEffect = nullptr;
    DeathEffect *m_deathEffect = nullptr;

    // —— 对话辅助 ——
    void playSequence(const QStringList &lines, std::function<void()> onDone);
    void playSingle(const QString &text, int gx, int gy, int maxChars,
                    std::function<void()> onDone);

    // —— 工具方法 ——
    /// ▶ 、 (1,0), ◀ 、 (-1,0), ▲ 、 (0,-1), ▼ 、 (0,1)
    static QPoint arrowDir(const QString &arrowText);

    /// 在场景中查找指定网格位置的 QGraphicsTextItem
    QGraphicsTextItem *itemAtGrid(QPoint gp, const QString &text = QString()) const;

    // —— 自动行走（Level9/Level12 共用） ——
    void autoWalkTo(QPoint target, std::function<void()> onDone);
    virtual void autoWalkStep();
    QTimer *m_autoWalkTimer = nullptr;
    QPoint m_autoWalkTarget{-1, -1};
    std::function<void()> m_autoWalkDone;
};

#endif // LEVELBASE_H
