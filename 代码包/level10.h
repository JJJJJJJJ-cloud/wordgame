#ifndef LEVEL10_H
#define LEVEL10_H

#include "levelbase.h"

#include <QGraphicsRectItem>
#include <QList>
#include <QSet>
#include <QPoint>
#include <QTimer>

class DialogueSystem;
class DeleteEffect;
class DeathEffect;

class Level10 : public LevelBase
{
    Q_OBJECT
public:
    explicit Level10(QObject *parent = nullptr);

    // 入场序列：简短对话 、 进入自由战斗
    void startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                    bool &inputEnabled);

    // Enter 键：弹反弹幕
    void handleEnterPress();

    // 按键追踪（由 GameView 转发）
    void keyPress(int key);
    void keyRelease(int key);

    // 查询：某 item 是否为龙（供 GameScene 保护不被误删）
    bool isDragonItem(QGraphicsTextItem *item) const { return item == m_dragon; }

    void cleanup();

signals:
    void level10Complete();

private:
    // 阶段
    enum class Phase { Entry, FreePlay, Win, Death };
    Phase m_phase = Phase::Entry;

    // Win 子阶段
    enum class WinStep {
        Shockwave,       // 冲击波扩散 + HUD淡出
        WaitForLake,     // 等待"未名湖"到达龙X
        BuildingsFrozen, // 建筑物冻结，开始对话
        Dialogue1,       // 第一段对话播放中
        DragonFall,      // 龙坠落
        Dialogue2,       // 第二段对话播放中
        FadeOut,         // 最终渐隐
        Done
    };
    WinStep m_winStep = WinStep::Shockwave;

    // 核心引用
    QGraphicsTextItem *m_player = nullptr;
    QGraphicsTextItem *m_arrow = nullptr;
    QGraphicsTextItem *m_dragon = nullptr;
    qreal m_dragonX = 0, m_dragonY = 0;   // 龙浮点像素坐标
    bool *m_inputEnabledPtr = nullptr;

    // 移动系统
    QSet<int> m_keysDown;
    QTimer *m_tickTimer = nullptr;

    static constexpr qreal kPlayerSpeed = 240.0;   // px/s
    static constexpr qreal kDragonSpeed = 100.0;   // px/s
    static constexpr qreal kBulletSpeed = 200.0;   // px/s = 0.2s/grid
    static constexpr int   kFireInterval = 500;    // ms between dragon shots

    // 弹幕
    struct Bullet {
        QGraphicsTextItem *item = nullptr;
        qreal x = 0, y = 0;    // 浮点像素坐标
        int dx = 0;            // +1 向右, -1 向左
        bool fromPlayer = false; // 被弹反过 、 可伤害龙
    };
    QList<Bullet> m_bullets;

    // HP
    int m_dragonHp = 20;
    int m_playerHp = 3;
    int m_fireCooldown = 0;    // ms until next dragon shot

    // 闪烁
    int m_dragonFlash = 0;     // 剩余闪烁 tick 数
    int m_playerFlash = 0;

    // HUD
    QGraphicsTextItem *m_dragonLabel = nullptr;
    QGraphicsRectItem *m_hpBarBg = nullptr;
    QGraphicsRectItem *m_hpBarFg = nullptr;
    QGraphicsTextItem *m_playerHpText = nullptr;

    static constexpr qreal kHpBarX = 80.0;
    static constexpr qreal kHpBarY = 325.0;
    static constexpr qreal kHpBarW = 200.0;
    static constexpr qreal kHpBarH = 15.0;

    // 建筑物背景（y=9~14 高速滚动）
    struct BuildingDef {
        QString text;          // 多字符字符串，每个字符占一行
        int startY;            // 起始 y 格坐标
        bool hasSubtitle;      // 是否带闪烁小字
        QString subtitleText;  // 小字内容
    };

    struct ScrollingBuilding {
        QList<QGraphicsTextItem*> items;  // 每个字一个 item
        QGraphicsTextItem *subtitleItem = nullptr;
        QString name;          // 建筑名称（如"未名湖"），用于胜利序列的冻结检测
        qreal x = 0;           // 当前浮点 x 像素坐标
    };

    QList<ScrollingBuilding> m_buildings;
    int m_buildingSpawnIndex = 0;        // 下一个要生成的建筑物序号
    bool m_buildingsInitialized = false;
    int m_buildingBlinkTick = 0;         // 小字闪烁计数器

    //胜利序列
    QList<QGraphicsEllipseItem*> m_shockwaveRings;  // 冲击波圆环
    bool m_buildingsFrozen = false;                  // 建筑物冻结标志

    static constexpr qreal kBuildingSpeed = 800.0;    // 20 格/秒
    static constexpr qreal kBuildingSpacing = 240.0;  // 6 格间距

    static const QList<BuildingDef> kBuildingDefs;

    void updateBuildings(qreal dt);
    void spawnBuilding(const BuildingDef &def, qreal x);
    void destroyBuilding(ScrollingBuilding &b);
    void resetBuildings();

    // 云和日（y=0~1 漂浮装饰）
    struct Cloud {
        QGraphicsTextItem *item = nullptr;
        qreal x = 0;
        int yGrid = 0;   // 0 或 1
    };
    QList<Cloud> m_clouds;
    QGraphicsTextItem *m_sun = nullptr;
    qreal m_cloudSpawnCooldown = 0;  // 下次生成云的冷却时间（秒）

    static constexpr qreal kCloudSpeed = 200.0;       // 5 格/秒
    static constexpr int   kMaxClouds = 5;            // 同屏最多云数
    static constexpr qreal kCloudSpawnInterval = 1.5; // 生成间隔（秒）

    void updateClouds(qreal dt);
    void spawnCloud();
    void resetClouds();

    // 内部方法
    void tick();
    void fireBullet();
    void checkCollisions();
    void updateHud();
    void onDragonHit();
    void onPlayerHit();
    void triggerWin();
    void triggerDeath();
    void moveDragon(qreal dt);
    void movePlayer(qreal dt);
    void moveBullets(qreal dt);
    void updateArrow();

    // —— 胜利序列 ——
    void startWinSequence();
    void startShockwave();
    void fadeOutHud();
    void doDragonFall();
    void finalFadeOut();
    void advanceWinSequence();
};

#endif
