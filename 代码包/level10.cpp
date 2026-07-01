#include "level10.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "deleteeffect.h"
#include "deatheffect.h"
#include "grid.h"

#include <QFont>
#include <QFontMetrics>
#include <QPropertyAnimation>
#include <QAbstractAnimation>
#include <QVariantAnimation>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>
#include <QPointer>
#include <QTimer>
#include <cmath>

//  建筑物定义（y=9~14 高速滚动背景）
const QList<Level10::BuildingDef> Level10::kBuildingDefs = {
    {QStringLiteral("五四体育场"), 10, false, QString()},
    {QStringLiteral("家园食堂"),   11, false, QString()},
    {QStringLiteral("理科教学楼"), 10, false, QString()},
    {QStringLiteral("校医院"),     12, false, QString()},
    {QStringLiteral("燕南食堂"),   11, false, QString()},
    {QStringLiteral("理科一号楼"), 10, false, QString()},
    {QStringLiteral("师"),         14, true,  QStringLiteral("快看！勇者！")},
    {QStringLiteral("学一食堂"),   11, false, QString()},
    {QStringLiteral("第二教学楼"), 10, false, QString()},
    {QStringLiteral("百讲"),       13, false, QString()},
    {QStringLiteral("博雅塔"),     12, false, QString()},
    {QStringLiteral("教师之家"),   11, false, QString()},
    {QStringLiteral("勺园食堂"),   11, false, QString()},
    {QStringLiteral("静园"),       13, false, QString()},
    {QStringLiteral("未名湖"),       12, false, QString()},
};

//  构造函数
Level10::Level10(QObject *parent)
    : LevelBase(parent)
{
}

//  入场序列
void Level10::startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                          bool &inputEnabled) {
    if (!m_scene || !player) return;
    m_phase = Phase::Entry;
    m_player = player;
    m_arrow = arrow;
    m_inputEnabledPtr = &inputEnabled;
    inputEnabled = false;

    // 找到场景中的龙（由 createLevel10 放置）
    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (ti && ti->toPlainText() == QStringLiteral("龙")) {
            QPoint g = Grid::toGrid(ti->pos());
            if (g == QPoint(0, 3)) {
                m_dragon = ti;
                m_dragonX = ti->pos().x();
                m_dragonY = ti->pos().y();
                break;
            }
        }
    }

    // 龙入场渐显
    if (m_dragon) {
        m_dragon->setOpacity(0.0);
        auto *dragonFade = new QPropertyAnimation(m_dragon, "opacity");
        dragonFade->setDuration(800);
        dragonFade->setEndValue(1.0);
        dragonFade->setEasingCurve(QEasingCurve::OutCubic);
        dragonFade->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // 玩家渐显
    m_player->setOpacity(0.0);
    auto *playerFade = new QPropertyAnimation(m_player, "opacity");
    playerFade->setDuration(800);
    playerFade->setEndValue(1.0);
    playerFade->setEasingCurve(QEasingCurve::OutCubic);
    playerFade->start(QAbstractAnimation::DeleteWhenStopped);

    // 箭头
    if (m_arrow) {
        m_arrow->setPos(m_player->pos() + QPointF(Grid::kSize, 0));
        m_arrow->setPlainText(QStringLiteral("▶"));
        m_arrow->setVisible(true);
    }

    // 初始化 HUD
    m_dragonHp = 20;
    m_playerHp = 3;
    m_fireCooldown = kFireInterval;
    updateHud();

    // 初始化云和日
    resetClouds();

    // 简短对话 、 开打
    if (m_dialogueSystem) {
        QStringList lines = {
            QStringLiteral("我：什么？我被传送到了学校上空！"),
            QStringLiteral("朗·昂索：此处甚好。让吾与汝，做个了断。"),
            QStringLiteral("*用恩特之盾弹反它的攻击吧！")
        };
        m_dialogueSystem->showSequence(lines, [this, &inputEnabled]() {
            m_phase = Phase::FreePlay;
            inputEnabled = true;

            // 初始化建筑物滚动
            resetBuildings();

            if (m_tickTimer) { m_tickTimer->stop(); delete m_tickTimer; }
            m_tickTimer = new QTimer(this);
            connect(m_tickTimer, &QTimer::timeout, this, &Level10::tick);
            m_tickTimer->start(16);  // ~60 FPS
        });
    } else {
        m_phase = Phase::FreePlay;
        inputEnabled = true;

        // 初始化建筑物滚动
        resetBuildings();

        if (m_tickTimer) { m_tickTimer->stop(); delete m_tickTimer; }
        m_tickTimer = new QTimer(this);
        connect(m_tickTimer, &QTimer::timeout, this, &Level10::tick);
        m_tickTimer->start(16);
    }
}

//  游戏循环 (16ms)
void Level10::tick() {
    if (m_phase != Phase::FreePlay && m_phase != Phase::Win) return;
    if (!m_player || !m_dragon) return;

    const qreal dt = 0.016;

    if (m_phase == Phase::FreePlay) {
        // 1. 移动玩家
        movePlayer(dt);

        // 2. 移动龙
        moveDragon(dt);

        // 3. 移动弹幕
        moveBullets(dt);

        // 4. 发射冷却
        m_fireCooldown -= 16;
        if (m_fireCooldown <= 0) {
            fireBullet();
            m_fireCooldown = kFireInterval;
        }

        // 5. 碰撞检测
        checkCollisions();

        // 6. 闪烁处理
        if (m_dragonFlash > 0) {
            m_dragonFlash--;
            m_dragon->setOpacity((m_dragonFlash % 2 == 0) ? 0.3 : 1.0);
            if (m_dragonFlash == 0) m_dragon->setOpacity(1.0);
        }
        if (m_playerFlash > 0) {
            m_playerFlash--;
            m_player->setOpacity((m_playerFlash % 2 == 0) ? 0.3 : 1.0);
            if (m_playerFlash == 0) m_player->setOpacity(1.0);
        }

        // 9. 更新箭头
        if (m_arrow) {
            QPoint ad = arrowDir(m_arrow->toPlainText());
            m_arrow->setPos(m_player->pos() + QPointF(ad.x() * Grid::kSize, ad.y() * Grid::kSize));
        }

        // 10. 更新 HUD
        updateHud();
    }

    // 两个阶段共享：建筑物和云始终滚动 
    // 7. 更新建筑物背景（y=9~14）
    updateBuildings(dt);

    // 8. 更新云和日（y=0~1 漂浮装饰）
    updateClouds(dt);
}

//  玩家移动（WASD 驱动）
void Level10::movePlayer(qreal dt) {
    if (!m_player) return;

    qreal vx = 0, vy = 0;
    if (m_keysDown.contains(Qt::Key_W)) vy = -kPlayerSpeed;
    if (m_keysDown.contains(Qt::Key_S)) vy = kPlayerSpeed;
    if (m_keysDown.contains(Qt::Key_A)) vx = -kPlayerSpeed;
    if (m_keysDown.contains(Qt::Key_D)) vx = kPlayerSpeed;

    if (vx != 0 && vy != 0) {
        // 斜向归一化（保持相同速率）
        vx *= 0.7071;
        vy *= 0.7071;
    }

    qreal newX = m_player->pos().x() + vx * dt;
    qreal newY = m_player->pos().y() + vy * dt;

    // Y 边界: 0 ~ 7 格
    if (newY < 0) newY = 0;
    if (newY > Grid::kSize * 7) newY = Grid::kSize * 7;

    // X 边界: 屏幕内
    if (newX < 0) newX = 0;
    if (newX > Grid::kSize * 19) newX = Grid::kSize * 19;

    m_player->setPos(newX, newY);

    // 更新箭头方向（按最后非零移动方向）
    if (vx > 0 && m_arrow)
        m_arrow->setPlainText(QStringLiteral("▶"));
    else if (vx < 0 && m_arrow)
        m_arrow->setPlainText(QStringLiteral("◀"));
    else if (vy < 0 && m_arrow)
        m_arrow->setPlainText(QStringLiteral("▲"));
    else if (vy > 0 && m_arrow)
        m_arrow->setPlainText(QStringLiteral("▼"));
}

//  龙 AI：追踪玩家
void Level10::moveDragon(qreal dt) {
    if (!m_dragon || !m_player) return;

    qreal dx = m_player->pos().x() - m_dragonX;
    qreal dy = m_player->pos().y() - m_dragonY;

    qreal dist = std::sqrt(dx * dx + dy * dy);
    qreal step = kDragonSpeed * dt;

    if (dist > step) {
        m_dragonX += (dx / dist) * step;
        m_dragonY += (dy / dist) * step;
    }

    // Y 边界: 0 ~ 7 格
    if (m_dragonY < 0) m_dragonY = 0;
    if (m_dragonY > Grid::kSize * 7) m_dragonY = Grid::kSize * 7;

    // X 边界
    if (m_dragonX < 0) m_dragonX = 0;
    if (m_dragonX > Grid::kSize * 19) m_dragonX = Grid::kSize * 19;

    m_dragon->setPos(m_dragonX, m_dragonY);
}

//  弹幕移动
void Level10::moveBullets(qreal dt) {
    for (int i = m_bullets.size() - 1; i >= 0; --i) {
        auto &b = m_bullets[i];
        if (!b.item) { m_bullets.removeAt(i); continue; }

        b.x += b.dx * kBulletSpeed * dt;
        b.item->setPos(b.x, b.y);

        // 碰到屏幕边缘消失
        if (b.x < -40 || b.x > Grid::kSize * 19 + 40) {
            m_scene->removeItem(b.item);
            delete b.item;
            m_bullets.removeAt(i);
        }
    }
}

//  龙发射弹幕
void Level10::fireBullet() {
    if (!m_scene || !m_dragon || !m_player) return;

    QFont bulletFont(QStringLiteral("黑体"), 18, QFont::Bold);
    auto *b = m_scene->addText(QStringLiteral("弹"), bulletFont);
    b->setDefaultTextColor(QColor(255, 200, 60));  // 橙黄色
    b->setPos(m_dragonX, m_dragonY);
    b->setZValue(30);

    // 发光特效
    auto *glow = new QGraphicsDropShadowEffect;
    glow->setBlurRadius(18);
    glow->setColor(QColor(255, 180, 40, 200));
    glow->setOffset(0, 0);
    b->setGraphicsEffect(glow);

    // 方向：向玩家发射（左右方向，取水平分量）
    int dx = (m_player->pos().x() >= m_dragonX) ? 1 : -1;

    Bullet bullet;
    bullet.item = b;
    bullet.x = m_dragonX;
    bullet.y = m_dragonY;
    bullet.dx = dx;
    bullet.fromPlayer = false;
    m_bullets.append(bullet);
}

//  碰撞检测
void Level10::checkCollisions() {
    if (!m_player || !m_dragon) return;

    const qreal hitRadius = Grid::kSize * 0.6;  // 碰撞半径

    for (int i = m_bullets.size() - 1; i >= 0; --i) {
        // 防御：如果阶段已不再是 FreePlay（triggerDeath/triggerWin 清空了列表
        // 或修改了阶段），立即退出循环，避免访问已清空的 m_bullets
        if (m_phase != Phase::FreePlay) return;

        auto &b = m_bullets[i];
        if (!b.item) { m_bullets.removeAt(i); continue; }

        if (b.fromPlayer) {
            // 弹反的弹 、 检测是否击中龙
            qreal dist = std::sqrt(
                (b.x - m_dragonX) * (b.x - m_dragonX) +
                (b.y - m_dragonY) * (b.y - m_dragonY));
            if (dist < hitRadius) {
                m_scene->removeItem(b.item);
                delete b.item;
                m_bullets.removeAt(i);
                onDragonHit();
                if (m_phase != Phase::FreePlay) return;  // onDragonHit 可能触发 triggerWin
                continue;
            }
        } else {
            // 龙的弹 、 检测是否击中玩家
            QPointF pp = m_player->pos();
            qreal dist = std::sqrt(
                (b.x - pp.x()) * (b.x - pp.x()) +
                (b.y - pp.y()) * (b.y - pp.y()));
            if (dist < hitRadius) {
                m_scene->removeItem(b.item);
                delete b.item;
                m_bullets.removeAt(i);
                onPlayerHit();
                if (m_phase != Phase::FreePlay) return;  // onPlayerHit 可能触发 triggerDeath
                continue;
            }
        }
    }
}

//  龙受伤
void Level10::onDragonHit() {
    m_dragonHp--;
    m_dragonFlash = 8;  // 8 tick 闪烁 (~128ms)
    updateHud();

    if (m_dragonHp <= 0) {
        triggerWin();
    }
}

//  玩家受伤
void Level10::onPlayerHit() {
    m_playerHp--;
    m_playerFlash = 8;
    updateHud();

    if (m_playerHp <= 0) {
        triggerDeath();
    }
}

//  Enter 弹反
void Level10::handleEnterPress() {
    if (m_phase != Phase::FreePlay) return;
    if (!m_player || !m_arrow) return;
    if (m_deathEffect && m_deathEffect->isPlaying()) return;  // 死亡演出进行中不响应

    // 读取箭头方向
    QPoint ad = arrowDir(m_arrow->toPlainText());

    // 箭头前方一格的网格坐标
    QPoint playerGrid = Grid::toGrid(m_player->pos());
    QPoint frontCell1 = playerGrid + ad;
    QPoint frontCell2 = frontCell1 + ad;

    // 检测范围：前方 2 格，垂直方向 ±0.5 格容差（扩大判定区域）
    qreal margin = Grid::kSize / 2;  // 半格容差
    qreal xMin, xMax, yMin, yMax;
    if (ad.x() != 0) {
        // 水平方向弹反：覆盖前方 2 格 + 垂直半格容差
        xMin = (ad.x() > 0) ? frontCell1.x() * Grid::kSize : frontCell2.x() * Grid::kSize;
        xMax = (ad.x() > 0) ? (frontCell2.x() + 1) * Grid::kSize : (frontCell1.x() + 1) * Grid::kSize;
        yMin = playerGrid.y() * Grid::kSize - margin;
        yMax = (playerGrid.y() + 1) * Grid::kSize + margin;
    } else {
        // 垂直方向弹反：覆盖前方 2 格 + 水平半格容差
        yMin = (ad.y() > 0) ? frontCell1.y() * Grid::kSize : frontCell2.y() * Grid::kSize;
        yMax = (ad.y() > 0) ? (frontCell2.y() + 1) * Grid::kSize : (frontCell1.y() + 1) * Grid::kSize;
        xMin = playerGrid.x() * Grid::kSize - margin;
        xMax = (playerGrid.x() + 1) * Grid::kSize + margin;
    }

    // 查找前方范围内的敌人弹幕
    for (int i = m_bullets.size() - 1; i >= 0; --i) {
        auto &b = m_bullets[i];
        if (!b.item || b.fromPlayer) continue;

        if (b.x >= xMin && b.x < xMax && b.y >= yMin && b.y < yMax) {
            // 弹反：反转方向
            b.dx = -b.dx;
            b.fromPlayer = true;
            b.item->setDefaultTextColor(QColor(100, 255, 200));  // 绿色
            if (m_deleteEffect)
                m_deleteEffect->playEnterShield(m_player);
            return;  // 每次只弹反一颗
        }
    }
}

//  HUD
void Level10::updateHud() {
    if (!m_scene) return;

    // 龙标签
    if (!m_dragonLabel) {
        m_dragonLabel = m_scene->addText(QStringLiteral("龙"), QFont(QStringLiteral("黑体"), 14));
        m_dragonLabel->setDefaultTextColor(QColor(255, 100, 100));
        m_dragonLabel->setZValue(300);
    }
    m_dragonLabel->setPos(kHpBarX - 30, kHpBarY - 3);

    //龙血量条背景
    if (!m_hpBarBg) {
        m_hpBarBg = m_scene->addRect(0, 0, kHpBarW, kHpBarH,
                                      QPen(QColor(80, 80, 80), 1.5),
                                      QBrush(QColor(40, 40, 40)));
        m_hpBarBg->setZValue(300);
    }
    m_hpBarBg->setPos(kHpBarX, kHpBarY);

    //龙血量条前景（更新而非重建，避免指针失效）
    qreal fillW = kHpBarW * (qreal(m_dragonHp) / 20.0);
    QColor fgColor = (m_dragonHp > 3) ? QColor(220, 60, 60)
                                      : QColor(255, 30, 30);
    if (!m_hpBarFg) {
        m_hpBarFg = m_scene->addRect(0, 0, fillW, kHpBarH,
                                      QPen(Qt::NoPen), QBrush(fgColor));
        m_hpBarFg->setZValue(301);
        m_hpBarFg->setPos(kHpBarX, kHpBarY);
    } else {
        m_hpBarFg->setRect(0, 0, fillW, kHpBarH);
        m_hpBarFg->setBrush(QBrush(fgColor));
    }

    //玩家血量
    QString hpText = QStringLiteral("我 x %1").arg(m_playerHp);
    if (!m_playerHpText) {
        m_playerHpText = m_scene->addText(hpText, QFont(QStringLiteral("黑体"), 18));
        m_playerHpText->setDefaultTextColor(QColor(255, 100, 100));
        m_playerHpText->setZValue(300);
    } else {
        m_playerHpText->setPlainText(hpText);
    }
    m_playerHpText->setPos(Grid::toPixel(14, 8));
}

//  胜利
void Level10::triggerWin() {
    if (m_phase == Phase::Win || m_phase == Phase::Death) return;
    m_phase = Phase::Win;
    m_winStep = WinStep::Shockwave;
    m_buildingsFrozen = false;

    // 清除剩余弹幕
    for (auto &b : m_bullets) {
        if (b.item && b.item->scene()) {
            b.item->scene()->removeItem(b.item);
            delete b.item;
        }
    }
    m_bullets.clear();

    if (m_inputEnabledPtr) *m_inputEnabledPtr = false;

    // 隐藏箭头（胜利演出不需要）
    if (m_arrow) m_arrow->setVisible(false);

    // ① 冲击波扩散
    startShockwave();

    // ② HUD 淡出
    fadeOutHud();

    // 冲击波完成后（~2s），过渡到等待"未名湖"
    QTimer::singleShot(2000, this, [this]() {
        if (m_phase == Phase::Win && m_winStep == WinStep::Shockwave)
            m_winStep = WinStep::WaitForLake;
    });

    // tick timer 保持运行（用于建筑和云滚动）
    // 建筑物将在 updateBuildings() 中检测"未名湖"到达 、 冻结
}

//  冲击波动画：3层圆环从龙中心扩散至全屏
void Level10::startShockwave() {
    if (!m_scene || !m_dragon) return;

    // 龙的中心点
    QFontMetrics fm(QFont(QStringLiteral("黑体"), 24));
    qreal dw = fm.horizontalAdvance(QStringLiteral("龙"));
    qreal dh = fm.height();
    QPointF center(m_dragonX + dw / 2.0, m_dragonY + dh / 2.0);

    // 场景对角线半径（确保覆盖全屏）
    qreal maxRadius = std::sqrt(800.0 * 800.0 + 600.0 * 600.0);

    // 三层圆环配置：颜色 + 延迟(ms)
    struct RingConfig {
        QColor color;
        int delayMs;
        qreal durationSec;
    };
    const RingConfig configs[] = {
        { QColor(255, 200, 60),   0,   1.0 },   // 金色
        { QColor(255, 140, 30),   150, 1.1 },   // 橙色
        { QColor(255, 60,  30),   300, 1.2 },   // 红色
    };

    for (const auto &cfg : configs) {
        auto *ring = m_scene->addEllipse(0, 0, 1, 1,
                                          QPen(cfg.color, 3.0), Qt::NoBrush);
        ring->setPos(center);
        ring->setZValue(200);
        ring->setOpacity(1.0);
        ring->setVisible(false);  // 延迟后显示
        m_shockwaveRings.append(ring);

        // 用 QVariantAnimation 驱动缩放
        auto *anim = new QVariantAnimation(this);
        anim->setStartValue(20.0);    // 起始半径
        anim->setEndValue(maxRadius); // 扩展到覆盖全屏
        anim->setDuration(int(cfg.durationSec * 1000));
        anim->setEasingCurve(QEasingCurve::OutCubic);

        QTimer::singleShot(cfg.delayMs, this, [ring, anim]() {
            ring->setVisible(true);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        });

        connect(anim, &QVariantAnimation::valueChanged, this,
                [ring, center](const QVariant &v) {
            qreal r = v.toDouble();
            QRectF rect(-r, -r, r * 2, r * 2);
            ring->setRect(rect);
            ring->setPos(center);
            // 同时渐隐：半径越大越透明
            qreal alpha = 1.0 - (r / 550.0);
            if (alpha < 0) alpha = 0;
            ring->setOpacity(alpha);
        });

        connect(anim, &QVariantAnimation::finished, this, [this, ring]() {
            // 只从场景移除，不 delete —— 由 cleanup() 统一负责删除，
            // 避免 cleanup 遍历 m_shockwaveRings 时访问已释放内存
            if (ring && ring->scene()) {
                ring->scene()->removeItem(ring);
            }
            m_shockwaveRings.removeAll(ring);
        });
    }
}

//  HUD 淡出（血条 + 龙标签 + 玩家生命计数）
void Level10::fadeOutHud() {
    auto fadeItem = [this](QGraphicsItem *item, int duration = 500) {
        if (!item) return;
        // QGraphicsItem（含 QGraphicsRectItem）不是 QObject，不能直接用
        // QPropertyAnimation / QPointer。用 QVariantAnimation 手动 setOpacity。
        auto *anim = new QVariantAnimation(this);
        anim->setDuration(duration);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        anim->setEasingCurve(QEasingCurve::InQuad);
        connect(anim, &QVariantAnimation::valueChanged, this,
                [item](const QVariant &v) {
            if (item->scene()) item->setOpacity(v.toDouble());
        });
        connect(anim, &QVariantAnimation::finished, this, [item]() {
            if (item->scene()) item->setVisible(false);
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    };

    fadeItem(m_hpBarBg);
    fadeItem(m_hpBarFg);
    fadeItem(m_dragonLabel);
    fadeItem(m_playerHpText);
}

//  推进胜利序列（由 updateBuildings 和对话回调触发）
void Level10::advanceWinSequence() {
    switch (m_winStep) {
    case WinStep::BuildingsFrozen: {
        // ④ 对话1：龙和玩家的对话
        m_winStep = WinStep::Dialogue1;
        if (m_dialogueSystem && m_scene) {
            QStringList lines = {
                QStringLiteral("朗·昂索：怎会...如此...？"),
                QStringLiteral("我：结束...了吗？")
            };
            m_dialogueSystem->showSequence(lines, [this]() {
                // 对话1结束 、 龙坠落
                m_dialogueSystem->cleanup();
                doDragonFall();
            });
        } else {
            doDragonFall();
        }
        break;
    }
    case WinStep::DragonFall: {
        //龙已坠地渐隐 、 对话2
        m_winStep = WinStep::Dialogue2;
        if (m_dialogueSystem && m_scene) {
            QStringList lines = {
                QStringLiteral("它落到未名湖边了！追上去看看！")
            };
            m_dialogueSystem->showSequence(lines, [this]() {
                // 对话2结束 、 渐隐
                m_dialogueSystem->cleanup();
                m_winStep = WinStep::FadeOut;
                finalFadeOut();
            });
        } else {
            m_winStep = WinStep::FadeOut;
            finalFadeOut();
        }
        break;
    }
    default:
        break;
    }
}

//  龙坠落：从战斗区坠落到"未名湖"的"未"处
void Level10::doDragonFall() {
    if (!m_dragon) return;

    // 目标Y："未名湖"的 startY=12 的"未"字格
    qreal targetY = Grid::toPixel(0, 12).y();

    // 目标X：保持龙的当前X（建筑已冻结在龙X附近）
    qreal targetX = m_dragonX;

    // 龙的初始位置保持X不变
    m_dragonX = targetX;

    auto *fallAnim = new QPropertyAnimation(m_dragon, "pos");
    fallAnim->setDuration(800);
    fallAnim->setStartValue(m_dragon->pos());
    fallAnim->setEndValue(QPointF(targetX, targetY));
    fallAnim->setEasingCurve(QEasingCurve::InQuad);

    connect(fallAnim, &QPropertyAnimation::finished, this, [this]() {
        // 龙渐隐
        auto *fadeOut = new QPropertyAnimation(m_dragon, "opacity");
        fadeOut->setDuration(600);
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InCubic);
        connect(fadeOut, &QPropertyAnimation::finished, this, [this]() {
            if (m_dragon && m_dragon->scene()) {
                m_dragon->scene()->removeItem(m_dragon);
                // 不 delete m_dragon，cleanup 负责
            }
            if (m_dragon) m_dragon->setVisible(false);
            m_winStep = WinStep::DragonFall;  // 切换到下一阶段
            advanceWinSequence();  // 、 对话2
        });
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    });

    fallAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

//  最终渐隐：全屏黑幕 、 关卡结束
void Level10::finalFadeOut() {
    if (!m_scene) {
        emit level10Complete();
        return;
    }

    if (m_tickTimer) { m_tickTimer->stop(); }

    QGraphicsRectItem *fadeRect = m_scene->addRect(m_scene->sceneRect(),
                                                    QPen(Qt::NoPen),
                                                    QBrush(Qt::black));
    fadeRect->setOpacity(0.0);
    fadeRect->setZValue(999);

    auto *fade = new QVariantAnimation(this);
    fade->setDuration(800);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::InQuad);

    connect(fade, &QVariantAnimation::valueChanged, this,
            [fadeRect](const QVariant &v) {
        fadeRect->setOpacity(v.toDouble());
    });

    connect(fade, &QVariantAnimation::finished, this, [this, fadeRect]() {
        m_scene->removeItem(fadeRect);
        delete fadeRect;
        emit level10Complete();
    });

    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

//  死亡
void Level10::triggerDeath() {
    // 防止重复触发（同一帧内多颗子弹命中时）
    if (m_phase == Phase::Death) return;

    m_phase = Phase::Death;
    if (m_tickTimer) { m_tickTimer->stop(); }

    // 清除剩余弹幕
    for (auto &b : m_bullets) {
        if (b.item && b.item->scene()) {
            b.item->scene()->removeItem(b.item);
            delete b.item;
        }
    }
    m_bullets.clear();

    if (m_inputEnabledPtr) *m_inputEnabledPtr = false;

    if (m_deathEffect && m_player) {
        QStringList deathDialogues = {
            QStringLiteral("你自高天陨落..."),
            QStringLiteral("愤怒与不甘，成为谁人那早已失神的眼底最后的沉淀...")
        };
        QStringList epilogueLines = {
            QStringLiteral("...但在此世，你依旧拥有重新来过的契机。"),
            QStringLiteral("再次启程，剑指那不容于理的存在。")
        };
        m_deathEffect->play(m_player, deathDialogues, epilogueLines,
                            [this]() {
            // ⚠️ 关键修复：用 singleShot(0) 延迟清理和重启，
            // 确保 DialogueSystem 的回调链完全退出后再操作，
            // 避免在对话回调栈内重入 cleanup() / disconnect()
            QTimer::singleShot(0, this, [this]() {
                // 先清理死亡遮罩和"赤"字
                if (m_deathEffect) m_deathEffect->cleanup();

                // 重置建筑物背景和云
                resetBuildings();
                resetClouds();

                // DeathEffect 结束 、 重置血量并重新开始
                m_dragonHp = 20;
                m_playerHp = 3;
                m_fireCooldown = kFireInterval;
                m_dragonFlash = 0;
                m_playerFlash = 0;
                m_keysDown.clear();
                updateHud();

                // 重置位置
                if (m_player) {
                    m_player->setPos(Grid::toPixel(16, 3));
                    m_player->setRotation(0);   // 取消 DeathEffect 的 90° 旋转
                    m_player->setOpacity(1.0);  // 恢复不透明度
                }
                if (m_dragon) {
                    m_dragonX = Grid::toPixel(0, 3).x();
                    m_dragonY = Grid::toPixel(0, 3).y();
                    m_dragon->setPos(m_dragonX, m_dragonY);
                    m_dragon->setOpacity(1.0);
                }
                if (m_arrow) {
                    m_arrow->setPlainText(QStringLiteral("▶"));
                    m_arrow->setVisible(true);
                }

                m_phase = Phase::FreePlay;
                if (m_inputEnabledPtr) *m_inputEnabledPtr = true;

                if (m_tickTimer) { m_tickTimer->stop(); delete m_tickTimer; }
                m_tickTimer = new QTimer(this);
                connect(m_tickTimer, &QTimer::timeout, this, &Level10::tick);
                m_tickTimer->start(16);
            });
        });
    } else {
        // 无 DeathEffect 时的回退
        if (m_dialogueSystem) {
            m_dialogueSystem->display(
                QStringLiteral("生命值耗尽，重新开始..."),
                3, 12, 16, [this]() {
                    // 同样延迟到下一事件循环，避免重入
                    QTimer::singleShot(0, this, [this]() {
                        // 重置建筑物背景和云
                        resetBuildings();
                        resetClouds();

                        m_dragonHp = 20;
                        m_playerHp = 3;
                        m_keysDown.clear();
                        if (m_player) {
                            m_player->setPos(Grid::toPixel(16, 3));
                            m_player->setRotation(0);
                            m_player->setOpacity(1.0);
                        }
                        if (m_dragon) {
                            m_dragonX = Grid::toPixel(0, 3).x();
                            m_dragonY = Grid::toPixel(0, 3).y();
                            m_dragon->setPos(m_dragonX, m_dragonY);
                        }
                        m_phase = Phase::FreePlay;
                        if (m_inputEnabledPtr) *m_inputEnabledPtr = true;
                        if (m_tickTimer) { m_tickTimer->stop(); delete m_tickTimer; }
                        m_tickTimer = new QTimer(this);
                        connect(m_tickTimer, &QTimer::timeout, this, &Level10::tick);
                        m_tickTimer->start(16);
                    });
                });
        }
    }
}

//  建筑物背景（y=9~14 高速滚动，20格/秒）
void Level10::spawnBuilding(const BuildingDef &def, qreal x) {
    if (!m_scene) return;

    ScrollingBuilding b;
    b.x = x;
    b.name = def.text;  // 存储名称，供胜利序列冻结检测使用

    QFont buildingFont(QStringLiteral("黑体"), 20);
    QColor buildingColor(150, 160, 175);  // 灰蓝色，远景感

    for (int i = 0; i < def.text.size(); ++i) {
        auto *item = m_scene->addText(QString(def.text[i]), buildingFont);
        item->setDefaultTextColor(buildingColor);
        item->setPos(x, Grid::toPixel(0, def.startY + i).y());
        item->setZValue(5);  // 底层，不遮挡游戏元素
        b.items.append(item);
    }

    // 闪烁小字（"师"的 "快看！勇者！"）
    if (def.hasSubtitle && !def.subtitleText.isEmpty()) {
        QFont subFont(QStringLiteral("黑体"), 10);
        auto *sub = m_scene->addText(def.subtitleText, subFont);
        sub->setDefaultTextColor(QColor(255, 220, 80));  // 金色
        // 放在建筑物右侧偏上
        sub->setPos(x + Grid::kSize, Grid::toPixel(0, def.startY).y() - 8);
        sub->setZValue(6);
        b.subtitleItem = sub;
    }

    m_buildings.append(b);
}

void Level10::destroyBuilding(ScrollingBuilding &b) {
    for (auto *item : b.items) {
        if (item && item->scene()) m_scene->removeItem(item);
        delete item;
    }
    b.items.clear();

    if (b.subtitleItem) {
        if (b.subtitleItem->scene()) m_scene->removeItem(b.subtitleItem);
        delete b.subtitleItem;
        b.subtitleItem = nullptr;
    }
}

void Level10::resetBuildings() {
    // 清理现有建筑物
    for (auto &b : m_buildings)
        destroyBuilding(b);
    m_buildings.clear();

    m_buildingSpawnIndex = 0;
    m_buildingsInitialized = false;
    m_buildingsFrozen = false;
}

void Level10::updateBuildings(qreal dt) {
    if (!m_scene) return;

    m_buildingBlinkTick++;

    // 首次调用：生成第一批建筑物
    if (!m_buildingsInitialized) {
        m_buildingsInitialized = true;
        qreal x = Grid::kSize * 21;  // 从屏幕右边缘外 1 格开始
        // 预生成 4 栋填满初始视野
        for (int i = 0; i < 4; ++i) {
            spawnBuilding(kBuildingDefs[m_buildingSpawnIndex], x);
            m_buildingSpawnIndex = (m_buildingSpawnIndex + 1) % kBuildingDefs.size();
            x += kBuildingSpacing;
        }
    }

    //胜利序列：检测"未名湖"到达龙X 、 冻结所有建筑
    if (m_phase == Phase::Win && m_winStep == WinStep::WaitForLake && !m_buildingsFrozen) {
        const qreal threshold = Grid::kSize * 1.5;  // 1.5 格容差
        for (const auto &b : m_buildings) {
            if (b.name == QStringLiteral("未名湖")
                && std::abs(b.x - m_dragonX) < threshold) {
                m_buildingsFrozen = true;
                m_winStep = WinStep::BuildingsFrozen;
                // 对齐：让"未"字正好在龙X处
                qreal snapX = m_dragonX;
                qreal dx = snapX - b.x;
                for (auto &bb : m_buildings) {
                    bb.x += dx;
                    for (auto *item : bb.items) {
                        if (item && item->scene()) item->setX(bb.x);
                    }
                    if (bb.subtitleItem && bb.subtitleItem->scene())
                        bb.subtitleItem->setX(bb.x + Grid::kSize);
                }
                advanceWinSequence();  // 、 对话1
                break;
            }
        }
    }

    // 冻结后跳过移动和生成
    if (m_buildingsFrozen) return;

    // 移动所有建筑物，移除已离开屏幕的
    for (int i = m_buildings.size() - 1; i >= 0; --i) {
        auto &b = m_buildings[i];
        b.x -= kBuildingSpeed * dt;

        // 更新各字符位置
        for (int j = 0; j < b.items.size(); ++j) {
            if (b.items[j] && b.items[j]->scene())
                b.items[j]->setX(b.x);
        }

        // 闪烁小字跟随，约每 500ms 切换可见性
        if (b.subtitleItem && b.subtitleItem->scene()) {
            b.subtitleItem->setX(b.x + Grid::kSize);
            // 用全局计数器产生 30 tick 周期 (~480ms) 的闪烁
            b.subtitleItem->setVisible((m_buildingBlinkTick % 30) < 15);
        }

        // 完全离开左屏幕后销毁
        if (b.x < -Grid::kSize * 4) {
            destroyBuilding(b);
            m_buildings.removeAt(i);
        }
    }

    // 补充新建筑物：当最右侧建筑进入屏幕一定距离后
    qreal rightEdge = Grid::kSize * 21;  // 屏幕右边缘 + 1 格
    if (!m_buildings.isEmpty()) {
        qreal rightmostX = m_buildings.last().x;
        if (rightmostX < rightEdge) {
            qreal spawnX = qMax(rightEdge, rightmostX + kBuildingSpacing);
            spawnBuilding(kBuildingDefs[m_buildingSpawnIndex], spawnX);
            m_buildingSpawnIndex = (m_buildingSpawnIndex + 1) % kBuildingDefs.size();
        }
    }
}

//  云和日（y=0~1 漂浮装饰，5格/秒，无碰撞）
void Level10::spawnCloud() {
    if (!m_scene) return;
    if (m_clouds.size() >= kMaxClouds) return;

    Cloud c;
    c.yGrid = (m_clouds.size() % 2 == 0) ? 0 : 1;  // 交替出现在 y=0 和 y=1
    c.x = Grid::kSize * 21 + (m_clouds.size() * 60); // 屏幕右外，错开出现

    QFont cloudFont(QStringLiteral("黑体"), 16);
    auto *item = m_scene->addText(QStringLiteral("云"), cloudFont);
    item->setDefaultTextColor(QColor(210, 215, 225, 180));  // 浅灰白，半透明感
    item->setPos(c.x, Grid::toPixel(0, c.yGrid).y());
    item->setZValue(3);  // 最底层装饰
    c.item = item;

    m_clouds.append(c);
}

void Level10::resetClouds() {
    // 清理现有云
    for (auto &c : m_clouds) {
        if (c.item && c.item->scene()) m_scene->removeItem(c.item);
        delete c.item;
    }
    m_clouds.clear();

    // 清理日
    if (m_sun) {
        if (m_sun->scene()) m_scene->removeItem(m_sun);
        delete m_sun;
        m_sun = nullptr;
    }

    m_cloudSpawnCooldown = 0;

    // 创建"日"在 (10, 0)
    if (m_scene) {
        QFont sunFont(QStringLiteral("黑体"), 22);
        m_sun = m_scene->addText(QStringLiteral("日"), sunFont);
        m_sun->setDefaultTextColor(QColor(255, 200, 60));  // 暖金色
        m_sun->setPos(Grid::toPixel(10, 0));
        m_sun->setZValue(3);  // 底层装饰
    }

    // 预生成 2 朵云填入视野
    spawnCloud();
    spawnCloud();
}

void Level10::updateClouds(qreal dt) {
    if (!m_scene) return;

    // 生成冷却
    m_cloudSpawnCooldown -= dt;
    if (m_cloudSpawnCooldown <= 0 && m_clouds.size() < kMaxClouds) {
        spawnCloud();
        m_cloudSpawnCooldown = kCloudSpawnInterval;
    }

    // 移动云朵，移除离开屏幕的
    for (int i = m_clouds.size() - 1; i >= 0; --i) {
        auto &c = m_clouds[i];
        if (!c.item || !c.item->scene()) {
            m_clouds.removeAt(i);
            continue;
        }

        c.x -= kCloudSpeed * dt;
        c.item->setX(c.x);

        // 添加微小上下浮动（±3px，用 sin 波）
        qreal floatOffset = 3.0 * std::sin(c.x * 0.02);
        c.item->setY(Grid::toPixel(0, c.yGrid).y() + floatOffset);

        // 离开左屏幕后回收
        if (c.x < -Grid::kSize * 2) {
            m_scene->removeItem(c.item);
            delete c.item;
            m_clouds.removeAt(i);
        }
    }
}

//  按键追踪
void Level10::keyPress(int key) {
    m_keysDown.insert(key);
}

void Level10::keyRelease(int key) {
    m_keysDown.remove(key);
}

//  清理
void Level10::cleanup() {
    if (m_tickTimer) { m_tickTimer->stop(); delete m_tickTimer; m_tickTimer = nullptr; }

    // 先停止所有子动画，防止动画回调访问已删除的 QGraphicsItem
    const auto anims = findChildren<QAbstractAnimation *>();
    for (auto *anim : anims) {
        anim->stop();
        anim->deleteLater();
    }

    for (auto &b : m_bullets) {
        if (b.item && b.item->scene()) {
            b.item->scene()->removeItem(b.item);
            delete b.item;
        }
    }
    m_bullets.clear();

    // 清理冲击波圆环
    for (auto *ring : m_shockwaveRings) {
        if (ring && ring->scene()) ring->scene()->removeItem(ring);
        delete ring;
    }
    m_shockwaveRings.clear();

    if (m_hpBarBg) {
        if (m_hpBarBg->scene()) m_hpBarBg->scene()->removeItem(m_hpBarBg);
        delete m_hpBarBg;
        m_hpBarBg = nullptr;
    }
    if (m_hpBarFg) {
        if (m_hpBarFg->scene()) m_hpBarFg->scene()->removeItem(m_hpBarFg);
        delete m_hpBarFg;
        m_hpBarFg = nullptr;
    }
    if (m_dragonLabel) {
        if (m_dragonLabel->scene()) m_dragonLabel->scene()->removeItem(m_dragonLabel);
        delete m_dragonLabel;
        m_dragonLabel = nullptr;
    }
    if (m_playerHpText) {
        if (m_playerHpText->scene()) m_playerHpText->scene()->removeItem(m_playerHpText);
        delete m_playerHpText;
        m_playerHpText = nullptr;
    }

    // 清理建筑物背景
    for (auto &b : m_buildings)
        destroyBuilding(b);
    m_buildings.clear();
    m_buildingsInitialized = false;
    m_buildingSpawnIndex = 0;

    // 清理云和日
    for (auto &c : m_clouds) {
        if (c.item && c.item->scene()) m_scene->removeItem(c.item);
        delete c.item;
    }
    m_clouds.clear();
    m_cloudSpawnCooldown = 0;
    if (m_sun) {
        if (m_sun->scene()) m_scene->removeItem(m_sun);
        delete m_sun;
        m_sun = nullptr;
    }

    m_keysDown.clear();
    m_phase = Phase::Entry;
    m_winStep = WinStep::Shockwave;
    m_buildingsFrozen = false;
    m_player = nullptr;
    m_arrow = nullptr;
    m_dragon = nullptr;
    m_inputEnabledPtr = nullptr;
    m_dragonHp = 20;
    m_playerHp = 3;
    m_dragonX = 0; m_dragonY = 0;
    m_fireCooldown = 0;
    m_dragonFlash = 0;
    m_playerFlash = 0;

    LevelBase::cleanup();
}
