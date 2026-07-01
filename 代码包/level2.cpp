#include "level2.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "grid.h"

#include <QGraphicsScene>
#include <QTimer>
#include <QPointer>
#include <QFont>
#include <cstdlib>

Level2::Level2(QObject *parent)
    : LevelBase(parent)
{
}

void Level2::startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                         bool &inputEnabled, bool &arrowEnabled,
                         std::function<void(const QString &, int, int,
                                            const QSet<int> &,
                                            std::function<void()>)> revealFn)
{
    if (!m_dialogueSystem || !player) return;

    inputEnabled = false;
    if (arrow) arrow->setVisible(false);

    m_dialogueSystem->display(
        QStringLiteral("（食堂排了好长的队...）"), 13, 11, 22,
        [=, &inputEnabled, &arrowEnabled]() {
            if (revealFn) {
                revealFn(
                    QStringLiteral("已经早上七点了，感觉十有八九还要再等会"),
                    1, 12,
                    {4, 10, 12, 13},  // "七","十","八","九"
                    [=, &inputEnabled, &arrowEnabled]() {
                        // 揭示完成后排队抱怨气泡
                        setupQueueBubbles();
                        inputEnabled = true;
                        if (arrow && arrowEnabled) arrow->setVisible(true);
                    }
                );
            }
        });
}

void Level2::showBubble(const QString &text, int gx, int gy, int stayMs, QColor color)
{
    if (!m_scene) return;
    QFont bf(QStringLiteral("黑体"), 14);
    auto *b = m_scene->addText(text, bf);
    b->setDefaultTextColor(color);
    b->setPos(Grid::toPixel(gx, gy));
    b->setOpacity(0.0);
    b->setZValue(5);
    QPointer<QGraphicsTextItem> bp(b);
    Anim::fade(m_scene, b, 1.0, 400, [this, bp, stayMs]() {
        if (!bp) return;
        QTimer::singleShot(stayMs, this, [this, bp]() {
            if (!bp) return;
            Anim::fade(m_scene, bp.data(), 0.0, 500, [this, bp]() {
                if (bp) { m_scene->removeItem(bp.data()); delete bp.data(); }
            });
        });
    });
}

void Level2::setupQueueBubbles()
{
    // 第一句抱怨，1.2s 后浮现
    QTimer::singleShot(1200, this, [this]() {
        showBubble(QStringLiteral("怎么这么慢啊..."), 12, 8, 2500,
                   QColor(220, 210, 180));
    });
    // 第二句抱怨，4s 后浮现
    QTimer::singleShot(4000, this, [this]() {
        showBubble(QStringLiteral("好饿好饿"), 14, 8, 2500,
                   QColor(220, 210, 180));
    });

    // 排队抱怨气氛定时器（随机气泡）
    auto *qt = new QTimer(this);
    connect(qt, &QTimer::timeout, this, [this]() {
        int rx = 11 + (std::rand() % 7);
        QStringList bubbles = {QStringLiteral("…"), QStringLiteral("唉"),
                               QStringLiteral("好慢"), QStringLiteral("饿"),
                               QStringLiteral("快点啊")};
        QString bubble = bubbles[std::rand() % bubbles.size()];
        QFont bf(QStringLiteral("黑体"), 12);
        auto *b = m_scene->addText(bubble, bf);
        b->setDefaultTextColor(QColor(200, 200, 180));
        b->setPos(Grid::toPixel(rx, 9));
        b->setZValue(5);
        QPointer<QGraphicsTextItem> bp(b);
        QTimer::singleShot(900, this, [this, bp]() {
            if (bp) { m_scene->removeItem(bp.data()); delete bp.data(); }
        });
    });
    qt->start(3500);
}

void Level2::cleanup()
{
    // 停止所有子定时器（QTimer 作为 QObject 子对象会被自动删除）
    for (auto *child : children()) {
        if (auto *t = qobject_cast<QTimer*>(child))
            t->stop();
    }
    LevelBase::cleanup();
}
