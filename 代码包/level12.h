#ifndef LEVEL12_H
#define LEVEL12_H

#include "levelbase.h"

#include <QList>
#include <QTimer>

class DialogueSystem;

class Level12 : public LevelBase
{
    Q_OBJECT
public:
    explicit Level12(QObject *parent = nullptr);

    void startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                    bool &inputEnabled);
    void cleanup() override;

signals:
    void level12Complete();

private:
    QGraphicsTextItem *m_player = nullptr;
    QGraphicsTextItem *m_arrow = nullptr;
    bool *m_inputEnabled = nullptr;
    QGraphicsTextItem *m_doorItem = nullptr;

    // 推字系统
    QTimer *m_pushTimer = nullptr;
    int m_pushIndex = 0;
    QList<QGraphicsTextItem*> m_pushChars;
    QString m_pushText;

    // 临时物品（用于 cleanup）
    QList<QGraphicsTextItem*> m_tempItems;
    QList<QTimer*> m_tempTimers;
    QList<QGraphicsTextItem*> m_newSceneItems;

    // 阶段
    void phase1_openingDialogue();
    void phase2_autoWalkToDoor();
    void phase3_roommateDialogue();
    void phase4_typewriterPush();
    void phase5_fadeOut();
    void phase6_fadeInNewScene();
    void phase7_slideInDreamChars();
    void phase8_slideInTeacher();
    void phase9_dreamDialogue();
    void phase10_finalText();
    void shakeScreen();

    void pushStep();
    void walkTo(int gx, int gy, std::function<void()> onDone);

    QGraphicsTextItem *slideInChar(const QString &ch, int targetGx, int gy,
                                   qreal startX, int delayMs,
                                   std::function<void()> onDone = nullptr);
};

#endif
