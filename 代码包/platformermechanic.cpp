#include "platformermechanic.h"
#include "grid.h"
#include "animutils.h"
#include "dialoguesystem.h"

#include <QGraphicsScene>
#include <QPropertyAnimation>
#include <QPointer>

#include <algorithm>
#include <cmath>

PlatformerMechanic::PlatformerMechanic(QObject *parent)
    : QObject(parent)
{
}

// 第六关入场叙事
void PlatformerMechanic::startLevel6Entry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                                           bool &inputEnabled, bool &arrowEnabled,
                                           QPoint climbWinPos, QPoint climbStartPos,
                                           bool &levelPassed)
{
    if (!m_scene || !player) return;

    arrowEnabled = false;
    if (arrow) arrow->setVisible(false);
    inputEnabled = false;

    // 先把渐进平台隐藏，避免在对话期间闪现
    for (auto *item : m_scene->items()) {
        if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
            if (ti->data(1).toBool()) {
                int gy = Grid::toGrid(ti->pos()).y();
                if (gy <= 10) ti->setVisible(false);
            }
        }
    }

    // 找老师
    QGraphicsTextItem *teacher = nullptr;
    for (auto *item : m_scene->items()) {
        if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
            if (ti->toPlainText() == QStringLiteral("师")) {
                teacher = ti;
                break;
            }
        }
    }

    player->setOpacity(0.0);
    if (teacher) teacher->setOpacity(0.0);

    auto *pFade = Anim::prop(m_scene, player, "opacity", 500, 1.0);
    auto *tFade = Anim::prop(m_scene, teacher, "opacity", 500, 1.0);
    QList<QPropertyAnimation*> anims{pFade};
    if (teacher) anims.append(tFade);

    QPointer<QGraphicsTextItem> pp(player);
    QPointer<QGraphicsTextItem> tp(teacher);

    Anim::parallel(m_scene, anims, [=, &inputEnabled, &levelPassed]() {
        if (!pp) return;
        if (!m_dialogueSystem) return;
        m_dialogueSystem->setSequencePos(1, 10, 24);
        QStringList lines = {
            QStringLiteral("老师：接下来我们去寻找恩特之盾。"),
            QStringLiteral("据说，它被尘封在博雅塔顶。"),
            QStringLiteral("我：但是，我们怎么上去呢？"),
            QStringLiteral("老师：看你的了，勇者！")
        };
        m_dialogueSystem->showSequence(lines, [=, &inputEnabled, &levelPassed]() {
            if (!pp) return;
            setInitialLayers(3);
            start(pp.data(), climbWinPos, climbStartPos, levelPassed, inputEnabled);
            inputEnabled = true;
        });
    });
}

// 启动/停止
void PlatformerMechanic::start(QGraphicsTextItem *player, const QPoint &climbWinPos,
                                const QPoint &climbStartPos, bool &levelPassed, bool &inputEnabled) {
    m_player = player;
    m_climbWinPos = climbWinPos;
    m_climbStartPos = climbStartPos;
    m_levelPassedPtr = &levelPassed;
    m_inputEnabledPtr = &inputEnabled;

    m_running = true;
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PlatformerMechanic::tick);
    m_timer->start(16);
    m_velX = 0; m_velY = 0;
    m_grounded = false;
    m_jumpPressed = false;
    m_floating = false;
    m_keysDown.clear();

    resetRevealState(player);
}

void PlatformerMechanic::stop() {
    m_running = false;
    if (m_timer) { m_timer->stop(); delete m_timer; m_timer = nullptr; }
}

void PlatformerMechanic::pause() {
    if (m_timer) m_timer->stop();
}

void PlatformerMechanic::resume() {
    if (m_timer) m_timer->start(16);
}

// 输入
void PlatformerMechanic::keyPress(int key) {
    m_keysDown.insert(key);
    if ((key == Qt::Key_Space || key == Qt::Key_W || key == Qt::Key_Up)
        && m_grounded && !m_jumpPressed) {
        m_jumpPressed = true;
        m_velY = kJumpVel;
        m_grounded = false;
    }
}

void PlatformerMechanic::keyRelease(int key) {
    m_keysDown.remove(key);
    if (key == Qt::Key_Space || key == Qt::Key_W || key == Qt::Key_Up)
        m_jumpPressed = false;
}

// 每帧更新
void PlatformerMechanic::tick() {
    if (!m_player || !m_levelPassedPtr || *m_levelPassedPtr) return;
    const qreal dt = 0.016;

    // 飘落状态：碰到云后缓慢匀速下降、无视输入，一路穿过中间平台直到底层
    if (m_floating) {
        m_velX = 0;
        m_velY = kFloatSpeed;
        m_player->setY(m_player->y() + m_velY * dt);
        m_grounded = false;
        m_groundPlatform = nullptr;

        // 飘到最底层平台才停下，并重置为初始状态（从头再来）
        QGraphicsTextItem *base = (m_layers.isEmpty() || m_layers.first().isEmpty())
                                      ? nullptr : m_layers.first().first();
        qreal stopTop = base ? base->pos().y() : 560.0;
        if (m_player->y() + m_player->boundingRect().height() >= stopTop) {
            m_player->setY(stopTop - m_player->boundingRect().height());
            m_velY = 0;
            m_floating = false;
            resetRevealState(m_player);          // 重新隐藏上层，从头再来
        }
        return;
    }

    m_velX = 0;
    if (m_keysDown.contains(Qt::Key_A) || m_keysDown.contains(Qt::Key_Left))
        m_velX = -kWalkSpeed;
    if (m_keysDown.contains(Qt::Key_D) || m_keysDown.contains(Qt::Key_Right))
        m_velX = kWalkSpeed;

    m_velY += kGravity * dt;

    qreal newX = m_player->x() + m_velX * dt;
    qreal newY = m_player->y() + m_velY * dt;
    m_player->setPos(newX, newY);
    m_grounded = false;
    m_groundPlatform = nullptr;

    checkLanding();

    if (m_player->x() < 0) m_player->setX(0);
    if (m_player->x() > 760) m_player->setX(760);

    // 掉落到底部
    if (m_player->y() > 620) {
        m_player->setPos(Grid::toPixel(m_climbStartPos.x(), m_climbStartPos.y()));
        m_velX = 0; m_velY = 0;
        resetRevealState(m_player);
    }

    // 到达顶部获胜：必须真正落在最顶层平台上（站稳即可，水平位置不限）
    if (m_climbWinPos.x() >= 0 && m_grounded && !m_layers.isEmpty() &&
        m_layers.last().contains(m_groundPlatform)) {
        *m_levelPassedPtr = true;
        stop();
        *m_inputEnabledPtr = false;
        emit winReached();
        return;
    }

    // 支线：碰到云 、 吐槽一句，并缓慢飘落到底层
    if (m_scene) {
        bool touchedCloudNow = false;
        const auto colliding = m_scene->collidingItems(m_player);
        for (auto *it : colliding) {
            if (it->data(0).toString() == QLatin1String("cloud")) {
                touchedCloudNow = true;
                break;
            }
        }
        if (touchedCloudNow) {
            m_floating = true;
            emit touchedCloud();
        }
    }
}

// 平台碰撞
bool PlatformerMechanic::checkLanding() {
    if (!m_player || m_velY < 0) return false;
    QRectF pr = m_player->boundingRect();
    QRectF playerRect(m_player->pos(), pr.size());
    qreal playerBottom = playerRect.bottom();

    for (auto *plat : m_platforms) {
        if (!plat || !m_activePlatforms.contains(plat)) continue;
        QRectF br = plat->boundingRect();
        QRectF platRect(plat->pos(), br.size());

        if (playerBottom >= platRect.top() &&
            playerBottom <= platRect.top() + m_velY * 0.016 + 12 &&
            playerRect.right() > platRect.left() + 4 &&
            playerRect.left() < platRect.right() - 4) {
            m_player->setY(platRect.top() - pr.height());
            m_velY = 0;
            m_grounded = true;
            m_groundPlatform = plat;

            // 落在当前最高层的任意一段 、 整层揭示下一层（左右两边一起出现）
            if (m_nextLayerIndex >= 1 && m_nextLayerIndex < m_layers.size() &&
                m_layers[m_nextLayerIndex - 1].contains(plat)) {
                for (auto *nextPlat : m_layers[m_nextLayerIndex]) {
                    nextPlat->setVisible(true);
                    m_activePlatforms.insert(nextPlat);
                }
                ++m_nextLayerIndex;
            }
            return true;
        }
    }
    return false;
}

// 平台揭示
void PlatformerMechanic::resetRevealState(QGraphicsTextItem *player) {
    m_activePlatforms.clear();
    m_layers.clear();
    m_nextLayerIndex = 0;

    m_platforms.clear();
    for (auto *item : m_scene->items()) {
        if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
            // 只收集被显式标记为平台的项(data(1))，排除玩家/箭头/云/对话等其它文字
            if (ti != player && ti->data(1).toBool())
                m_platforms.append(ti);
        }
    }

    std::sort(m_platforms.begin(), m_platforms.end(), [](QGraphicsTextItem *a, QGraphicsTextItem *b) {
        return Grid::toGrid(a->pos()).y() > Grid::toGrid(b->pos()).y();
    });

    for (auto *plat : m_platforms)
        plat->setVisible(false);

    // 按相同高度（grid y）分组成层，从下到上
    for (auto *plat : m_platforms) {
        int gy = Grid::toGrid(plat->pos()).y();
        if (m_layers.isEmpty() ||
            Grid::toGrid(m_layers.last().first()->pos()).y() != gy) {
            m_layers.append(QList<QGraphicsTextItem*>{plat});
        } else {
            m_layers.last().append(plat);
        }
    }

    // 开局先揭示最底两层：玩家还没起跳时，从下往上第2层就已出现
    int initialLayers = std::min(m_initialLayers, static_cast<int>(m_layers.size()));
    for (int i = 0; i < initialLayers; ++i) {
        for (auto *plat : m_layers[i]) {
            plat->setVisible(true);
            m_activePlatforms.insert(plat);
        }
    }
    m_nextLayerIndex = initialLayers;
}

// 清理
void PlatformerMechanic::cleanup() {
    if (m_timer) { m_timer->stop(); delete m_timer; m_timer = nullptr; }
    m_running = false;
    m_platforms.clear();
    m_layers.clear();
    m_activePlatforms.clear();
    m_nextLayerIndex = 0;
    m_groundPlatform = nullptr;
    m_keysDown.clear();
    m_velX = 0; m_velY = 0;
    m_grounded = false;
    m_jumpPressed = false;
    m_floating = false;
    m_player = nullptr;
    m_levelPassedPtr = nullptr;
    m_inputEnabledPtr = nullptr;
}
