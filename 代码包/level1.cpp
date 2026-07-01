#include "level1.h"
#include "dialoguesystem.h"

#include <QGraphicsScene>

Level1::Level1(QObject *parent)
    : LevelBase(parent)
{
}

void Level1::startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                         bool &inputEnabled, bool &arrowEnabled,
                         std::function<void(const QString &, int, int,
                                            const QSet<int> &,
                                            std::function<void()>)> revealFn)
{
    if (!m_dialogueSystem || !player) return;

    inputEnabled = false;
    if (arrow) arrow->setVisible(false);

    m_dialogueSystem->display(
        QStringLiteral("（迷迷糊糊地从床上醒来...）"), 2, 12, 22,
        [=, &inputEnabled, &arrowEnabled]() {
            // 对话结束后逐字揭示独白
            if (revealFn) {
                revealFn(
                    QStringLiteral("哎，没什么头绪，先出门吧"),
                    3, 12,
                    {2},  // "没"字可推动
                    nullptr  // 无回调：揭示完毕后自动恢复输入 + 显示箭头
                );
            }
        });
}
