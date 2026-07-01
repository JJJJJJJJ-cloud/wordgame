#ifndef GAMESCENE_H
#define GAMESCENE_H

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QList>
#include <QMap>
#include <QSet>
#include <QPoint>
#include <QTimer>

#include "leveldata.h"
#include "savemanager.h"

class QParallelAnimationGroup;
class QAbstractAnimation;
class DialogueSystem;
class PauseMenu;
class Level3Road;
class Level4Classroom;
class FishingMechanic;
class PlatformerMechanic;
class Level7;
class Level8;
class Level9;
class Level10;
class Level11;
class Level12;
class DeleteEffect;
class DeathEffect;
class InteractionSystem;
class Level1;
class Level2;

class GameScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit GameScene(QObject *parent = nullptr);

    void movePlayer(int dx, int dy);
    void deleteForward();
    void handleEnterKey();
    void handleFKey();
    void loadLevel(int level);
    void checkLevelComplete();

    int currentLevel() const { return m_currentLevel; }
    LevelData::Mechanic currentMechanic() const { return m_mechanic; }

    // 对话系统（转发）
    bool isDialogueActive() const;
    void dialogueAdvance();

    // 文字逐个揭示系统
    bool isRevealing() const { return m_isRevealing; }
    void revealAdvance();
    void showDialogueSequence(const QStringList &lines, std::function<void()> onDone = nullptr);
    void showInteractiveText(const QString &text, int startGx, int gy,
                             const QSet<int> &pushableIndices);
    // 供 Level 子类调用的文字揭示入口
    void startTextReveal(const QString &text, int startGx, int gy,
                         const QSet<int> &pushableIndices,
                         std::function<void()> onDone = nullptr);

    // F 键交互
    bool isInteractionActive() const;
    void interactionAdvance();

    // 暂停菜单
    bool isPaused() const;
    bool isChoiceActive() const { return m_choiceActive; }
    void togglePause();
    void pauseKeyPress(int key);
    void choiceKeyPress(int key);

    // 钓鱼（转发）
    void fishingDropRope();
    void fishingCleanup();

    // 平台跳跃（转发）
    void platformerKeyPress(int key);
    void platformerKeyRelease(int key);

    void fadeTransition(int nextLevel);

    // —— 公有访问器（供子系统使用） ——
    QGraphicsTextItem *playerItem() const { return m_player; }
    QList<QGraphicsTextItem*> &pushableItems() { return m_pushables; }
    QList<QGraphicsTextItem*> &waterItems() { return m_waters; }
    QList<QGraphicsTextItem*> &specialItems() { return m_specialItems; }
    QList<QGraphicsTextItem*> &trapItems() { return m_traps; }
    DialogueSystem *dialogueSystem() const { return m_dialogueSystem; }
    PauseMenu *pauseMenu() const { return m_pauseMenu; }

    void setInputEnabled(bool enabled) { m_inputEnabled = enabled; }
    bool isInputEnabled() const { return m_inputEnabled; }

    // Level 10 特定 API
    bool isLevel10Active() const { return m_currentLevel == 10; }
    void level10KeyPress(int key);
    void level10KeyRelease(int key);

signals:
    void levelComplete();    // 关卡通关 / 返回主菜单
    void gameFinished();     // 仅 Level 12 通关后发射，用于触发通关名单
    void backToMenu();
    void levelChanged(int level);

private:
    //基础成员
    QGraphicsTextItem *m_player = nullptr;
    QGraphicsTextItem *m_arrow = nullptr;
    bool m_arrowEnabled = true;   // 当前关卡是否使用方向箭头（钓鱼/爬塔/3/4关不用）
    QList<QGraphicsTextItem*> m_pushables;
    QList<QGraphicsTextItem*> m_waters;
    QList<QGraphicsTextItem*> m_specialItems;
    QList<QGraphicsTextItem*> m_traps;


    int m_currentLevel = 1;
    bool m_levelPassed = false;
    bool m_moving = false;
    bool m_inputEnabled = true;
    int m_dirX = 1, m_dirY = 0;
    int m_loadGeneration = 0;  // 递增以作废旧 QTimer::singleShot 回调

    // 关卡配置
    LevelData::Mechanic m_mechanic = LevelData::Standard;
    LevelData::WinCondition m_winCondition = LevelData::HasText;
    QString m_winTarget;
    bool m_hasWinAnimation = false;
    QList<QPoint> m_winFrameWords;

    QList<ComboRule> m_comboRules;
    QList<PhraseCheckRule> m_phraseRules;

    //爬塔
    QPoint m_climbWinPos{-1, -1};
    QPoint m_climbStartPos{2, 13};
    QSet<QPair<int,int>> m_trapSet;

    //暂停菜单（GameScene 保留的状态）
    bool m_paused = false;
    QPointF m_savedPlayerPos;
    int m_savedDirX = 1, m_savedDirY = 0;

    //二选一（第一关坏结局等）
    bool m_choiceActive = false;
    int m_choiceIndex = 0;
    QList<QGraphicsTextItem*> m_choiceItems;
    std::function<void(int)> m_choiceCallback;
    void showChoice(const QString &prompt, const QString &opt0, const QString &opt1,
                    std::function<void(int)> callback);
    void clearChoice();

    //子系统
    DialogueSystem *m_dialogueSystem;
    PauseMenu *m_pauseMenu;
    Level1 *m_level1;
    Level2 *m_level2;
    Level3Road *m_level3Road;
    Level4Classroom *m_level4Classroom;
    FishingMechanic *m_fishingMechanic;
    PlatformerMechanic *m_platformerMechanic;
    Level7 *m_level7;
    Level8 *m_level8;
    Level9 *m_level9;
    Level10 *m_level10;
    Level11 *m_level11;
    Level12 *m_level12 = nullptr;
    DeleteEffect *m_deleteEffect;
    DeathEffect *m_deathEffect;
    InteractionSystem *m_interactionSystem;

    //碰撞检测
    bool isBlockedByWater(const QPoint &gridPos) const;
    bool isBlockedByPushable(const QPoint &gridPos, QGraphicsTextItem *exclude = nullptr) const;
    bool isOutOfBounds(const QPoint &gridPos) const;
    QGraphicsTextItem *pushableAtGrid(const QPoint &gridPos) const;
    QGraphicsTextItem *itemAtGrid(const QPoint &gridPos, const QString &text = QString()) const;

    //内部方法
    void updateArrow();
    void handleTextCombine(QGraphicsTextItem *movedItem);
    void checkPhraseFormation();
    void onPhraseWin();

    void startAutoWalk(int targetX, int targetY);
    void autoWalkStep();
    void cancelAutoWalk();
    void onAutoWalkComplete();

    void createFlashingHint(const QString &text, int gx, int gy, int fontSize = 14);
    void destroyHint();

    void triggerGameOver(const QString &message);
    void fadeOutItems(const QList<QPoint> &positions, std::function<void()> onDone);
    void cleanupLevelSpecific();
    void resetLevel();
    SaveData collectSaveData() const;
    QGraphicsTextItem *addSwayingDecor(const QString &text, int gx, int gy);

    QString arrowCharForDir(int dx, int dy) const;

    //自动行走
    QTimer *m_autoWalkTimer = nullptr;
    QPoint m_autoWalkTarget{-1, -1};
    QList<QPoint> m_autoWalkPath;
    QList<QPoint> findAutoWalkPath(QPoint from, QPoint to);

    //提示闪烁
    QGraphicsTextItem *m_hintItem = nullptr;
    QTimer *m_hintTimer = nullptr;

    //装饰动画
    QList<QAbstractAnimation*> m_decorAnims;

    //第二关钟表
    QGraphicsEllipseItem *m_clockFace = nullptr;
    QGraphicsLineItem *m_clockHand = nullptr;
    void animateClockHand(int targetHour, std::function<void()> onDone);

    //文字逐个揭示
    struct RevealChar {
        QString text;
        int gx, gy;
        bool pushable;
    };
    QList<RevealChar> m_revealQueue;
    int m_revealIndex = 0;
    QTimer *m_revealTimer = nullptr;
    bool m_isRevealing = false;
    std::function<void()> m_revealDoneCb;
    QGraphicsTextItem *m_revealCursor = nullptr;  // 揭示完毕后的句末倒三角

    void startReveal(const QList<RevealChar> &chars, std::function<void()> onDone = nullptr);
    void revealStep();
    void skipReveal();
    void showRevealCursor();   // 显示揭示系统句末光标
    void hideRevealCursor();   // 隐藏揭示系统句末光标
};

#endif
