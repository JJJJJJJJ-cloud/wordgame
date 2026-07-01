#include "level4classroom.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "leveldata.h"
#include "grid.h"
#include "achievement.h"

#include <QFont>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QTimer>

Level4Classroom::Level4Classroom(QObject *parent)
    : LevelBase(parent)
{
}

// 入场序列
void Level4Classroom::startEntry(QGraphicsTextItem *player, bool &inputEnabled) {
    if (!player) return;
    m_phase = 0;
    inputEnabled = false;

    // 渐显
    Anim::fade(this, player, 1.0, 600, [=, &inputEnabled]() {
        // 移动到 (17,1)
        Anim::slide(this, player, Grid::toPixel(17, 1), 400, [=, &inputEnabled]() {
            if (!m_dialogueSystem) return;
            m_dialogueSystem->display("老师：要上课了，快去找个位置坐下吧。", 0, 11, 18, [=, &inputEnabled]() {
                if (!m_dialogueSystem) return;
                m_dialogueSystem->display("我：哦哦，好的！", 0, 11, 18, [=, &inputEnabled]() {
                    m_phase = 1;
                    inputEnabled = true;
                    emit entryComplete();
                });
            });
        });
    });
}

// 进度触发
bool Level4Classroom::checkProgressTrigger(QGraphicsTextItem *player,
                                            const QPoint &triggerGrid,
                                            bool &inputEnabled) {
    if (m_phase != 1) return false;
    if (!player) return false;
    QPoint pg = Grid::toGrid(player->pos());
    if (pg != triggerGrid) return false;

    m_phase = 2;
    inputEnabled = false;
    // 叙事由 GameScene 触发（需要先 destroyHint），此处只返回 true 表示触发
    return true;
}

// 叙事序列
void Level4Classroom::doNarrative(std::function<void()> onDone) {
    LevelData data = createLevel4();
    if (m_dialogueSystem) {
        m_dialogueSystem->showSequence(data.dialogues, std::move(onDone));
    }
}

// 教师行走
void Level4Classroom::doTeacherWalk(QList<QGraphicsTextItem*> &waters,
                                     QGraphicsTextItem *player,
                                     std::function<void()> onDone) {
    // 找到 "师" 字
    QGraphicsTextItem *shi = nullptr;
    for (auto *w : waters) {
        if (w->toPlainText() == "师") { shi = w; break; }
    }
    if (!shi) { onDone(); return; }

    auto *seq = new QSequentialAnimationGroup(this);
    seq->addAnimation(Anim::prop(this, shi, "pos", 500, Grid::toPixel(8, 1), QEasingCurve::Linear));
    seq->addAnimation(Anim::prop(this, shi, "pos", 800, Grid::toPixel(8, 9), QEasingCurve::Linear));
    seq->addAnimation(Anim::prop(this, shi, "pos", 700, Grid::toPixel(13, 9), QEasingCurve::Linear));

    connect(seq, &QSequentialAnimationGroup::finished, this, [=]() {
        if (!player) { onDone(); return; }
        Anim::parallel(this, {
            Anim::prop(this, shi, "opacity", 800, 0.0),
            Anim::prop(this, player, "opacity", 800, 0.0)
        }, [onDone]() { onDone(); });
    });
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

// 教室布景：三排"人"
void Level4Classroom::setupClassroom(QList<QGraphicsTextItem*> &waters) {
    QFont f("黑体", 24);

    auto addPerson = [&](int gx, int gy) {
        auto *p = m_scene->addText("人", f);
        p->setDefaultTextColor(Qt::white);
        p->setPos(Grid::toPixel(gx, gy));
        waters.append(p);  // 有碰撞体积
    };

    struct { int gy; QList<int> gxs; } rows[] = {
        {4, {2,3,4,5,6,7, 9,10,11,12,13,14,15}},
        {6, {2,3,4,5,6,7, 9,10,11,12,13,14,15}},
        {8, {2,3,4,5,6,7, 9,10,11,12, 14,15}},  // 排除 gx=13
    };

    for (const auto &row : rows) {
        for (int gx : row.gxs) {
            addPerson(gx, row.gy);
        }
    }

    // 启动 ZZZ 循环（z 字在 (gx=2, gy=3) 上方浮现）
    m_zzzPhase = 0;
    m_zzzTimer = new QTimer(this);
    connect(m_zzzTimer, &QTimer::timeout, this, &Level4Classroom::zzzTick);
    m_zzzTimer->start(500);

    // —— 打字"嗒"动画：从 (gx=4, gy=9) 向右 ——
    m_typeSpawnIndex = 0;
    m_typeTimer = new QTimer(this);
    connect(m_typeTimer, &QTimer::timeout, this, [this, &waters]() {
        typeTick(waters);
    });
    m_typeTimer->start(80);
}

// ZZZ 循环
void Level4Classroom::zzzTick() {
    const int Z_CNT = 3;
    if (m_zzzPhase < Z_CNT) {
        // 出现第 m_zzzPhase 个 z
        QFont zFont("黑体", 14 + m_zzzPhase * 4);  // 从小到大：14, 18, 22
        QGraphicsTextItem *z = m_scene->addText("z", zFont);
        z->setDefaultTextColor(QColor(200, 200, 255));
        QPointF basePos = Grid::toPixel(2, 3);
        z->setPos(basePos.x() - (m_zzzPhase + 1) * 18, basePos.y() - 8);
        z->setOpacity(0.0);
        m_zzzItems.append(z);

        // 渐显
        Anim::fade(this, z, 1.0, 200);

        m_zzzPhase++;
    } else if (m_zzzPhase == Z_CNT) {
        // 三个 z 都出现了，等 0.5s 后全部消失
        m_zzzPhase++;
    } else {
        // 全部消失
        for (auto *z : m_zzzItems) {
            Anim::fade(this, z, 0.0, 300, [this, z]() {
                if (z->scene()) z->scene()->removeItem(z);
                delete z;
            });
        }
        m_zzzItems.clear();
        m_zzzPhase = 0;
    }
}

// 绊线检测
void Level4Classroom::checkTripWire(const QPoint &playerGrid, QGraphicsTextItem *player,
                                     QList<QGraphicsTextItem*> &waters, bool &inputEnabled) {
    if (m_wireTriggered) return;
    if (playerGrid.y() != 4) return;
    int gx = playerGrid.x();
    if (gx < 16 || gx > 19) return;

    triggerTripWire(player, waters, inputEnabled);
}

void Level4Classroom::triggerTripWire(QGraphicsTextItem *player,
                                       QList<QGraphicsTextItem*> &waters, bool &inputEnabled) {
    m_wireTriggered = true;
    AchievementManager::instance()->unlock(AchievementId::BaoLiuJieMu);
    inputEnabled = false;

    QFont f("黑体", 24);
    // 在 (gx=16..19, gy=4) 创建"线"
    for (int gx = 16; gx <= 19; ++gx) {
        auto *w = m_scene->addText("线", f);
        w->setDefaultTextColor(QColor(255, 200, 100));
        w->setPos(Grid::toPixel(gx, 4));
        m_wireItems.append(w);
        // 初始无碰撞，对话后才加入 waters
    }

    // 玩家下移一格
    QPointF playerPos = player->pos();
    QPointF downPos = playerPos + QPointF(0, Grid::kSize);

    player->setTransformOriginPoint(player->boundingRect().center());
    Anim::parallel(this, {
        Anim::prop(this, player, "pos", 250, downPos, QEasingCurve::InQuad),
        Anim::prop(this, player, "rotation", 250, 90, QEasingCurve::InQuad)
    }, [=, &waters, &inputEnabled]() {
        // 播放三句对话
        if (!m_dialogueSystem) return;
        QStringList lines = {
            "哎呀！",
            "同学：不好意思，这里是我的电源线……",
            "算了算了，没事……"
        };
        m_dialogueSystem->showSequence(lines, [this, player, &waters, &inputEnabled]() {
            // 对话完成后，"线" 获得碰撞体积
            for (auto *w : m_wireItems) {
                waters.append(w);
            }
            // 玩家旋转恢复 + 恢复输入
            player->setTransformOriginPoint(player->boundingRect().center());
            auto *rReset = Anim::prop(this, player, "rotation", 400, 0, QEasingCurve::OutBack);
            connect(rReset, &QPropertyAnimation::finished, this, [&inputEnabled]() {
                inputEnabled = true;
            });
            rReset->start(QAbstractAnimation::DeleteWhenStopped);
        });
    });
}

// 打字"嗒"
void Level4Classroom::typeTick(QList<QGraphicsTextItem*> &waters) {
    const int total = 5;
    QFont f("黑体", 14);
    const QColor typeColor(180, 250, 180);
    int gy = 9;

    if (m_typeSpawnIndex == 0) {
        // "(键盘音)" — 和 "嗒" 同样大小和颜色，在 "嗒" 左边
        auto *tag = m_scene->addText("(键盘音)", f);
        tag->setDefaultTextColor(typeColor);
        tag->setPos(Grid::toPixel(3, gy));
        tag->setData(0, gy * Grid::kSize);
        m_typeChars.append(tag);
        m_typeSpawnIndex++;
        return;
    }

    int charIdx = m_typeSpawnIndex - 1;  // 0..4
    if (charIdx >= total) {
        m_typeTimer->stop();
        m_bounceIndex = 0;
        if (!m_bounceTimer) {
            m_bounceTimer = new QTimer(this);
            connect(m_bounceTimer, &QTimer::timeout, this, &Level4Classroom::bounceTick);
        }
        m_bounceTimer->start(300);
        return;
    }

    int gx = 5 + charIdx;
    auto *ch = m_scene->addText("嗒", f);
    ch->setDefaultTextColor(typeColor);
    ch->setPos(Grid::toPixel(gx, gy));
    ch->setData(0, gy * Grid::kSize);
    m_typeChars.append(ch);
    waters.append(ch);
    m_typeSpawnIndex++;
}

void Level4Classroom::bounceTick() {
    if (m_typeChars.isEmpty()) {
        if (m_bounceTimer) m_bounceTimer->stop();
        return;
    }
    int idx = m_bounceIndex % m_typeChars.size();
    QGraphicsTextItem *ch = m_typeChars[idx];
    if (ch) {
        qreal baseY = ch->data(0).toDouble();
        Anim::slide(this, ch, QPointF(ch->pos().x(), baseY - 6), 120, nullptr, QEasingCurve::OutQuad);
        // 保持轻微偏移感，不恢复原位
    }
    m_bounceIndex++;
    // 循环：所有字弹完后自动从头开始
}

// 清理
void Level4Classroom::cleanup() {
    m_phase = 0;

    // ZZZ
    if (m_zzzTimer) { m_zzzTimer->stop(); delete m_zzzTimer; m_zzzTimer = nullptr; }
    for (auto *z : m_zzzItems) {
        if (z->scene()) z->scene()->removeItem(z);
        delete z;
    }
    m_zzzItems.clear();
    m_zzzPerson = nullptr;
    m_zzzPhase = 0;

    // 绊线
    m_wireTriggered = false;
    for (auto *w : m_wireItems) {
        if (w->scene()) w->scene()->removeItem(w);
        delete w;
    }
    m_wireItems.clear();

    // 打字
    if (m_typeTimer) { m_typeTimer->stop(); delete m_typeTimer; m_typeTimer = nullptr; }
    if (m_bounceTimer) { m_bounceTimer->stop(); delete m_bounceTimer; m_bounceTimer = nullptr; }
    for (auto *ch : m_typeChars) {
        if (ch->scene()) ch->scene()->removeItem(ch);
        delete ch;
    }
    m_typeChars.clear();
    m_typeSpawnIndex = 0;
    m_bounceIndex = 0;
    LevelBase::cleanup();
}
