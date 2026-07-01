#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QList>
#include <QColor>

class QPushButton;

// 菜单调色板：集中管理所有颜色，支持绿/金两套主题
struct MenuPalette {
    QColor title;         // 标题主色
    QColor btn[3];        // 三个按钮默认色
    QColor btnHover;      // 高亮色
    QColor unlocked;      // 已解锁文字
    QColor completed;     // 已通关文字
    QColor accent;        // 暗色强调
    QColor accentDeep;    // 最深强调
    QColor grid;          // 网格线
    QColor towerLine;     // 博雅塔线
    QColor towerFill;     // 博雅塔填充
    QColor artifactText;  // 神器文字
    QColor terminalText;  // 终端文字
    QColor lakeText;      // 未名湖文字
    QColor memoryTitle;   // 回忆视图标题
    QColor memoryHint;    // 回忆视图副标题/底部提示
};

class MainMenu : public QWidget
{
    Q_OBJECT
public:
    explicit MainMenu(QWidget *parent = nullptr);

    void resetToMainState();
    void setVolumePercent(int vol) { m_volumePercent = qBound(0, vol, 100); }

signals:
    void startGameClicked();
    void loadLevelRequested(int level);
    void volumeChanged(int newVolume);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    // —— 状态 ——
    enum class MenuState { Main, MemorySelect, Achievements, SettingsVolume };
    MenuState m_menuState = MenuState::Main;

    // —— 按钮 ——
    QPushButton *m_startBtn;
    QPushButton *m_memoryBtn;
    QPushButton *m_achievementsBtn;
    QPushButton *m_settingsBtn;
    QPushButton *m_exitBtn;

    // —— 键盘导航 ——
    int m_keyboardSelection = 0;   // 主状态：0=开始游戏, 1=查看回忆, 2=游戏设置
    int m_memorySelection = 0;     // 回忆状态：选中的关卡索引（始终指向 unlocked 项）

    // —— Konami 码 ——
    QList<int> m_keyBuffer;
    bool m_konamiActivated = false;
    int m_konamiFlashTick = 0;
    QTimer *m_konamiFlashTimer;

    // —— 回忆数据 ——
    struct MemoryEntry {
        QString name;
        int level;
        bool unlocked;
        bool completed;
    };
    QList<MemoryEntry> m_memoryEntries;
    void refreshMemoryEntries();

    // —— 导航方法 ——
    void enterMemoryView();
    void enterAchievementsView();
    void enterSettingsVolume();
    void activateCurrentSelection();
    void updateButtonStyles();
    void refreshPalette();

    // —— 绘制 ——
    void drawGrid(QPainter &p);
    void drawDragonOnTower(QPainter &p);
    void drawTitle(QPainter &p);
    void drawIntro(QPainter &p);
    void drawBoyaTower(QPainter &p);
    void drawWeimingLake(QPainter &p);
    void drawArtifacts(QPainter &p);
    void drawTerminal(QPainter &p);
    void drawScanlines(QPainter &p);
    void drawVignette(QPainter &p);
    void drawMemoryView(QPainter &p);
    void drawAchievementsView(QPainter &p);
    void drawSettingsVolume(QPainter &p);

    // —— 通关后主题 ——
    bool m_isPostGame = false;
    MenuPalette m_pal;
    int m_volumePercent = 50;

    // —— 已有成员 ——
    QTimer *m_blinkTimer;
    bool m_cursorVisible = true;
};

#endif // MAINMENU_H
