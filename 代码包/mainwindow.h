#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <functional>

class MainMenu;
class GameView;
class GameScene;
class PrologueWidget;
class MusicManager;
class CreditsWidget;
struct AchievementInfo;

// 成就解锁弹窗 
class AchievementToast : public QWidget
{
    Q_OBJECT
public:
    explicit AchievementToast(const AchievementInfo &info, int stackIndex, QWidget *parent = nullptr);

    static void showToast(QWidget *parent, const AchievementInfo &info);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_name;
    QString m_desc;
    int m_stackIndex;
    QTimer *m_hideTimer = nullptr;

    void startShowAnimation();
    void startHideAnimation();
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void startGame(int level = 1);
    void onLevelComplete();
    void onGameFinished();
    void onLevelChanged(int level);
    void onAchievementUnlocked(const AchievementInfo &info);
    void onCreditsFinished();

private:
    void startPrologue();
    void onPrologueFinished();
    void runFadeToBlack(int durationMs, std::function<void()> onDone);
    void runFadeFromBlack(int durationMs, std::function<void()> onDone);

    QStackedWidget *m_stack;
    MainMenu *m_menu;
    PrologueWidget *m_prologue;
    GameView *m_gameView;
    GameScene *m_gameScene;
    MusicManager *m_musicManager;
    CreditsWidget *m_creditsWidget;
    QWidget *m_fadeOverlay;

    // Toast 堆叠管理
    int m_toastStackCount = 0;
};

#endif
