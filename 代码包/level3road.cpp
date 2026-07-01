#include "level3road.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "grid.h"

#include <QFont>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>

Level3Road::Level3Road(QObject *parent)
    : LevelBase(parent)
{
}

// 初始化
void Level3Road::setup(QList<QGraphicsTextItem*> &pushables,
                        QList<QGraphicsTextItem*> &waters,
                        QList<QGraphicsTextItem*> &specialItems) {
    Q_UNUSED(specialItems);
    m_roadBlocked = false;
    setupCars();
    QTimer::singleShot(400, this, [this, &pushables, &waters]() {
        setupDialogue(pushables, waters);
    });
}

// 道路车流
void Level3Road::setupCars() {
    QFont f("黑体", 24);

    for (int x = 0; x <= 19; ++x) {
        auto *r1 = m_scene->addText("路", f); r1->setDefaultTextColor(Qt::white);
        r1->setPos(Grid::toPixel(x, 4));
        auto *r2 = m_scene->addText("路", f); r2->setDefaultTextColor(Qt::white);
        r2->setPos(Grid::toPixel(x, 9));
    }

    struct CarCfg { int y; int startX; int endX; int dur; };
    QList<CarCfg> cars = {
        {5, 19, -2, 4500}, {5, 13, -2, 7000},
        {6, 16, -2, 5200}, {6, 8, -2, 6300},
        {7, -2, 21, 4800}, {7, 5, 21, 7200},
        {8, -2, 21, 5500}, {8, 6, 21, 6700},
    };

    for (auto &c : cars) {
        auto *car = m_scene->addText("车", f);
        car->setDefaultTextColor(Qt::white);
        car->setPos(Grid::toPixel(c.startX, c.y));
        m_cars.append(car);

        auto *anim = new QPropertyAnimation(car, "pos", this);
        anim->setDuration(c.dur);
        anim->setStartValue(Grid::toPixel(c.startX, c.y));
        anim->setEndValue(Grid::toPixel(c.endX, c.y));
        anim->setLoopCount(-1);
        anim->start();
        m_carAnims.append(anim);
    }
}

// 交互对话
void Level3Road::setupDialogue(QList<QGraphicsTextItem*> &pushables,
                                QList<QGraphicsTextItem*> &waters) {
    m_interactiveLineIdx = 0;

    // 前 3 句使用 DialogueSystem 打字机 + 句末倒三角
    // 第 4 句"一定要找到车！"作为场景字符放置，其中"找"可推动
    m_dialogueSystem->display("要迟到了，赶紧去上课吧。", 1, 11, 24,
        [this, &pushables, &waters]() {
        m_interactiveLineIdx = 1;
        m_dialogueSystem->display("等等，路上的车也太多了吧！", 1, 11, 24,
            [this, &pushables, &waters]() {
            m_interactiveLineIdx = 2;
            m_dialogueSystem->display("可我的车还停在对面...", 1, 11, 24,
                [this, &pushables, &waters]() {
                m_interactiveLineIdx = 3;
                // 最后一句：场景字符，"车"(i=5) 可推动
                QString lastLine = "一定要找到车！";
                for (int i = 0; i < lastLine.size(); ++i) {
                    auto *ch = m_scene->addText(QString(lastLine[i]),
                                                QFont("黑体", 24));
                    ch->setDefaultTextColor(Qt::white);
                    ch->setPos(Grid::toPixel(1 + i, 11));
                    m_interactiveChars.append(ch);
                    if (i == 5) {
                        pushables.append(ch);
                        m_interactivePushableIndices.insert(i);
                    } else {
                        waters.append(ch);
                    }
                }
            });
        });
    });
}

void Level3Road::advanceInteractiveDialogue(QList<QGraphicsTextItem*> &pushables,
                                             QList<QGraphicsTextItem*> &waters) {
    Q_UNUSED(pushables); Q_UNUSED(waters);
    // 不再使用：对话推进现在由 DialogueSystem::display() 自动处理
}

void Level3Road::clearInteractiveLine(QList<QGraphicsTextItem*> *pushables,
                                       QList<QGraphicsTextItem*> *waters) {
    for (auto *ch : m_interactiveChars) {
        if (waters)   waters->removeAll(ch);
        if (pushables) pushables->removeAll(ch);
        m_scene->removeItem(ch);
        delete ch;
    }
    m_interactiveChars.clear();
    m_interactivePushableIndices.clear();
}

bool Level3Road::hasInteractiveDialogue() const {
    return m_dialogueSystem && m_dialogueSystem->isActive();
}

void Level3Road::resetInteractiveDialogue() {
    m_interactiveLineIdx = 0;
    clearInteractiveLine();
    if (m_dialogueSystem)
        m_dialogueSystem->clearInteractiveAdvanceCallback();
}

// 道路阻挡
bool Level3Road::handleRoadBlock(QGraphicsTextItem *player) {
    if (m_roadBlocked) return true;  // 弹跳动画进行中，仍然阻止
    m_roadBlocked = true;
    QPointF orig = player->pos();
    Anim::slide(this, player, orig - QPointF(0, 15), 80, [=]() {
        Anim::slide(this, player, orig, 120, [=]() {
            m_roadBlocked = false;
        }, QEasingCurve::InQuad);
    }, QEasingCurve::OutQuad);
    showRoadWarning();
    return true;
}

void Level3Road::showRoadWarning() {
    if (m_warningTimer && m_warningTimer->isActive()) {
        if (m_warning) m_warning->setOpacity(1.0);
        m_warningTimer->start(2000);
        return;
    }

    if (!m_warning) {
        m_warning = m_scene->addText("前面的区域，以后再来探索吧？", QFont("黑体", 16));
        m_warning->setDefaultTextColor(QColor(255, 100, 100));
        m_warning->setPos(Grid::toPixel(10, 14));
    }
    m_warning->setOpacity(1.0);

    if (!m_warningTimer) {
        m_warningTimer = new QTimer(this);
        m_warningTimer->setSingleShot(true);
        connect(m_warningTimer, &QTimer::timeout, this, [=]() {
            if (!m_warning) return;
            Anim::fade(this, m_warning, 0.0, 500);
        });
    }
    m_warningTimer->start(2000);
    emit warningShown();
}

// 胜利检测
void Level3Road::checkWin(QGraphicsTextItem *player,
                           QList<QGraphicsTextItem*> &pushables,
                           QList<QGraphicsTextItem*> &waters,
                           QList<QGraphicsTextItem*> &specialItems,
                           bool &levelPassed, bool &inputEnabled,
                           std::function<void()> onFadeTransition) {
    Q_UNUSED(pushables); Q_UNUSED(waters);
    if (levelPassed) return;
    if (m_interactiveLineIdx < 3) return;

    QString phrase;
    for (int x = 0; x <= 8; ++x) {
        QGraphicsTextItem *found = nullptr;
        for (auto *ch : m_interactiveChars) {
            if (Grid::toGrid(ch->pos()) == QPoint(x, 11)) { found = ch; break; }
        }
        if (found)
            phrase += found->toPlainText();
        else if (Grid::toGrid(player->pos()) == QPoint(x, 11))
            phrase += "我";
        else
            break;
    }

    if (phrase != "车一定要找到我！") return;

    levelPassed = true;
    inputEnabled = false;

    QGraphicsTextItem *pickupCar = nullptr;
    for (auto *sp : specialItems) {
        if (sp->toPlainText() == "车") { pickupCar = sp; break; }
    }
    if (!pickupCar) return;

    QPointF dest = player->pos() + QPointF(-Grid::kSize, 0);
    Anim::slide(this, pickupCar, dest, 1000, [=]() {
        QPointF offScreen(Grid::toPixel(21, 13));
        Anim::parallel(this, {
            Anim::prop(this, player, "pos", 1500, offScreen, QEasingCurve::InQuad),
            Anim::prop(this, pickupCar, "pos", 1500, offScreen + QPointF(Grid::kSize, 0), QEasingCurve::InQuad)
        }, [onFadeTransition]() { onFadeTransition(); });
    }, QEasingCurve::InOutQuad);
}

// 清理
void Level3Road::cleanup() {
    for (auto *a : m_carAnims) { a->stop(); delete a; }
    m_carAnims.clear();
    for (auto *c : m_cars) { if (c->scene()) c->scene()->removeItem(c); delete c; }
    m_cars.clear();
    clearInteractiveLine();
    m_interactiveLines.clear();
    m_interactiveLineIdx = 0;
    if (m_warningTimer) { m_warningTimer->stop(); delete m_warningTimer; m_warningTimer = nullptr; }
    if (m_warning) { if (m_warning->scene()) m_warning->scene()->removeItem(m_warning); delete m_warning; m_warning = nullptr; }
    m_roadBlocked = false;
    if (m_dialogueSystem) m_dialogueSystem->clearInteractiveAdvanceCallback();
    LevelBase::cleanup();
}
