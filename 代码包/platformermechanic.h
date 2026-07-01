#ifndef PLATFORMERMECHANIC_H
#define PLATFORMERMECHANIC_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QSet>
#include <QList>
#include <QPoint>

class PlatformerMechanic : public QObject
{
    Q_OBJECT
public:
    explicit PlatformerMechanic(QObject *parent = nullptr);

    void setScene(QGraphicsScene *scene) { m_scene = scene; }
    void setDialogueSystem(class DialogueSystem *ds) { m_dialogueSystem = ds; }

    void start(QGraphicsTextItem *player, const QPoint &climbWinPos,
               const QPoint &climbStartPos, bool &levelPassed, bool &inputEnabled);
    void stop();
    void pause();
    void resume();
    bool isActive() const { return m_running; }
    void setInitialLayers(int n) { m_initialLayers = n; }

    void keyPress(int key);
    void keyRelease(int key);

    void resetRevealState(QGraphicsTextItem *player);
    void cleanup();

    // 第六关入场叙事：师徒渐显 、 对话 、 开始爬塔
    void startLevel6Entry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                          bool &inputEnabled, bool &arrowEnabled,
                          QPoint climbWinPos, QPoint climbStartPos, bool &levelPassed);

signals:
    void winReached();
    void touchedCloud();

private:
    QGraphicsScene *m_scene = nullptr;
    class DialogueSystem *m_dialogueSystem = nullptr;

    bool m_running = false;
    QTimer *m_timer = nullptr;
    QSet<int> m_keysDown;
    QList<QGraphicsTextItem*> m_platforms;          // 所有平台，按高度从下到上排序
    QList<QList<QGraphicsTextItem*>> m_layers;      // 按相同高度分组的层（从下到上）
    QSet<QGraphicsTextItem*> m_activePlatforms;     // 当前已揭示、可落脚的平台
    int m_nextLayerIndex = 0;                       // 下一个待揭示的层
    QGraphicsTextItem *m_groundPlatform = nullptr;  // 本帧落脚的平台（用于登顶判定）
    qreal m_velX = 0, m_velY = 0;
    bool m_grounded = false;
    bool m_jumpPressed = false;
    bool m_floating = false;         // 碰到云后的缓慢飘落状态

    // 由 start() 设置的外部引用
    QGraphicsTextItem *m_player = nullptr;
    QPoint m_climbWinPos{-1, -1};
    QPoint m_climbStartPos{2, 13};
    bool *m_levelPassedPtr = nullptr;
    bool *m_inputEnabledPtr = nullptr;

    int m_initialLayers = 2;

    static constexpr qreal kGravity = 760.0;
    static constexpr qreal kJumpVel = -420.0;
    static constexpr qreal kWalkSpeed = 240.0;
    static constexpr qreal kFloatSpeed = 160.0;  // 碰云后缓慢飘落的下降速度

    void tick();
    bool checkLanding();
};

#endif
