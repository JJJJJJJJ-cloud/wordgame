#include "mainwindow.h"
#include "mainmenu.h"
#include "prologuewidget.h"
#include "gameview.h"
#include "gamescene.h"
#include "musicmanager.h"
#include "savemanager.h"
#include "achievement.h"
#include "creditswidget.h"

#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QPainter>
#include <QFont>
#include <QDebug>

//  AchievementToast  成就解锁右下角弹窗

AchievementToast::AchievementToast(const AchievementInfo &info, int stackIndex, QWidget *parent)
    : QWidget(parent)
    , m_name(info.name)
    , m_desc(info.description)
    , m_stackIndex(stackIndex)
{
    setFixedSize(290, 72);
    setAttribute(Qt::WA_TransparentForMouseEvents);  // 不阻挡点击

    startShowAnimation();

    // 4 秒后自动隐藏
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, &AchievementToast::startHideAnimation);
    m_hideTimer->start(4500);
}

void AchievementToast::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 背景
    p.setPen(QPen(QColor(180, 150, 70), 1.5));
    p.setBrush(QColor(20, 20, 35, 230));
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);

    // 左侧奖杯图标区
    QRect iconRect(10, 10, 40, 50);
    QFont iconFont(QStringLiteral("黑体"), 22);
    p.setFont(iconFont);
    p.setPen(QColor(255, 215, 0));
    p.drawText(iconRect, Qt::AlignCenter, QStringLiteral("★"));  // ★

    // 右侧文字区
    QFont nameFont(QStringLiteral("黑体"), 14, QFont::Bold);
    p.setFont(nameFont);
    p.setPen(QColor(255, 225, 140));
    p.drawText(QRect(56, 12, 220, 22), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("成就解锁：") + m_name);

    QFont descFont(QStringLiteral("黑体"), 10);
    p.setFont(descFont);
    p.setPen(QColor(200, 200, 200));
    // 可能换行
    QRect descRect(56, 36, 220, 32);
    p.drawText(descRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, m_desc);
}

void AchievementToast::startShowAnimation() {
    // 定位在右下角，根据 stackIndex 向上偏移
    int baseX = parentWidget()->width() - 300;
    int baseY = parentWidget()->height() - 90 - m_stackIndex * 78;
    QPoint endPos(baseX, baseY);
    QPoint startPos(baseX + 310, baseY); // 从右侧屏幕外滑入

    move(startPos);
    QWidget::show();
    raise();

    auto *anim = new QPropertyAnimation(this, "pos");
    anim->setDuration(350);
    anim->setStartValue(startPos);
    anim->setEndValue(endPos);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void AchievementToast::startHideAnimation() {
    QPoint curPos = pos();
    QPoint endPos(curPos.x() + 310, curPos.y());

    auto *anim = new QPropertyAnimation(this, "pos");
    anim->setDuration(300);
    anim->setStartValue(curPos);
    anim->setEndValue(endPos);
    anim->setEasingCurve(QEasingCurve::InCubic);
    connect(anim, &QPropertyAnimation::finished, this, &QWidget::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void AchievementToast::showToast(QWidget *parent, const AchievementInfo &info) {
    // 统计当前已有的 toast 数量（作为堆叠序号）
    int count = 0;
    for (auto *child : parent->children()) {
        if (qobject_cast<AchievementToast*>(child))
            count++;
    }
    auto *toast = new AchievementToast(info, count, parent);
    toast->raise();
}

//  MainWindow

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    m_menu = new MainMenu(this);
    m_stack->addWidget(m_menu);

    m_prologue = new PrologueWidget(this);
    m_stack->addWidget(m_prologue);

    m_gameScene = new GameScene(this);
    m_gameView = new GameView(this);
    m_gameView->setScene(m_gameScene);
    m_stack->addWidget(m_gameView);

    // 通关名单页面
    m_creditsWidget = new CreditsWidget(this);
    m_stack->addWidget(m_creditsWidget);
    connect(m_creditsWidget, &CreditsWidget::finished, this, &MainWindow::onCreditsFinished);

    connect(m_menu, &MainMenu::startGameClicked, this, &MainWindow::startPrologue);
    connect(m_menu, &MainMenu::loadLevelRequested, this, &MainWindow::startGame);
    connect(m_prologue, &PrologueWidget::finished, this, &MainWindow::onPrologueFinished);

    // 音乐管理器
    m_musicManager = new MusicManager(this);
    connect(m_gameScene, &GameScene::levelChanged, this, &MainWindow::onLevelChanged);
    connect(m_gameScene, &GameScene::gameFinished, this, &MainWindow::onGameFinished);
    connect(m_menu, &MainMenu::volumeChanged, this, [this](int vol) {
        m_musicManager->setVolume(vol);
    });

    // 成就管理器初始化 & 信号连接
    AchievementManager::instance()->init();
    connect(AchievementManager::instance(), &AchievementManager::achievementUnlocked,
            this, &MainWindow::onAchievementUnlocked);

    // 启动时根据通关状态播放菜单音乐
    if (SaveManager::exists("slot12"))
        m_musicManager->playPostGameMenuMusic();
    else
        m_musicManager->playMenuMusic();

    // 淡入淡出遮罩（置于 stacked widget 之上，初始隐藏）
    m_fadeOverlay = new QWidget(this);
    m_fadeOverlay->setStyleSheet("background: black;");
    m_fadeOverlay->setGeometry(rect());
    m_fadeOverlay->hide();

    m_stack->setCurrentWidget(m_menu);
    showFullScreen();

    // 主菜单渐显
    runFadeFromBlack(800, nullptr);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_fadeOverlay)
        m_fadeOverlay->setGeometry(rect());

    // 重定位所有 toast
    int idx = 0;
    for (auto *child : children()) {
        if (auto *toast = qobject_cast<AchievementToast*>(child)) {
            toast->move(width() - 300, height() - 90 - idx * 78);
            idx++;
        }
    }
}

// 序幕：打字机演出
void MainWindow::startPrologue()
{
    // 将通关状态传递给序幕，使其配色与菜单一致
    m_prologue->setPostGameMode(SaveManager::exists("slot12"));

    runFadeToBlack(500, [this]() {
        m_stack->setCurrentWidget(m_prologue);
        m_prologue->start();
        runFadeFromBlack(500, nullptr);
    });
}

// 序幕结束 、 进入第一关
void MainWindow::onPrologueFinished()
{
    m_prologue->stop();

    disconnect(m_gameScene, &GameScene::levelComplete, nullptr, nullptr);
    connect(m_gameScene, &GameScene::levelComplete, this, &MainWindow::onLevelComplete);
    disconnect(m_gameScene, &GameScene::backToMenu, nullptr, nullptr);
    connect(m_gameScene, &GameScene::backToMenu, this, &MainWindow::onLevelComplete);

    runFadeToBlack(500, [this]() {
        m_gameScene->loadLevel(1);
        m_stack->setCurrentWidget(m_gameView);
        m_gameView->setFocus();
        runFadeFromBlack(500, nullptr);
    });
}

// 正常开始游戏（跳过序幕，供回忆/暂停菜单使用）
void MainWindow::startGame(int level)
{
    disconnect(m_gameScene, &GameScene::levelComplete, nullptr, nullptr);
    connect(m_gameScene, &GameScene::levelComplete, this, &MainWindow::onLevelComplete);
    disconnect(m_gameScene, &GameScene::backToMenu, nullptr, nullptr);
    connect(m_gameScene, &GameScene::backToMenu, this, &MainWindow::onLevelComplete);

    m_gameScene->loadLevel(level);
    m_stack->setCurrentWidget(m_gameView);
    m_gameView->setFocus();
}

// 通关 / 返回主菜单
void MainWindow::onLevelComplete()
{
    qDebug() << "[MainWindow] onLevelComplete() called — returning to main menu";
    runFadeToBlack(500, [this]() {
        m_stack->setCurrentWidget(m_menu);
        m_menu->resetToMainState();
        m_menu->setFocus();

        if (SaveManager::exists("slot12"))
            m_musicManager->playPostGameMenuMusic();
        else
            m_musicManager->playMenuMusic();

        runFadeFromBlack(500, nullptr);
    });
}

// Level 12 通关 、 播放制作人名单
void MainWindow::onGameFinished()
{
    m_stack->setCurrentWidget(m_creditsWidget);
    m_creditsWidget->start();
    m_creditsWidget->setFocus();
}

void MainWindow::onCreditsFinished()
{
    runFadeToBlack(500, [this]() {
        m_stack->setCurrentWidget(m_menu);
        m_menu->resetToMainState();
        m_menu->setFocus();
        m_musicManager->playPostGameMenuMusic();
        runFadeFromBlack(500, nullptr);
    });
}

// 通用淡入淡出遮罩
void MainWindow::runFadeToBlack(int durationMs, std::function<void()> onDone)
{
    auto *effect = new QGraphicsOpacityEffect(m_fadeOverlay);
    effect->setOpacity(0.0);
    m_fadeOverlay->setGraphicsEffect(effect);
    m_fadeOverlay->setGeometry(rect());
    m_fadeOverlay->raise();
    m_fadeOverlay->show();

    auto *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(durationMs);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::InQuad);

    connect(anim, &QPropertyAnimation::finished, this, [this, onDone, anim, effect]() {
        if (onDone) onDone();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::runFadeFromBlack(int durationMs, std::function<void()> onDone)
{
    auto *effect = new QGraphicsOpacityEffect(m_fadeOverlay);
    effect->setOpacity(1.0);
    m_fadeOverlay->setGraphicsEffect(effect);
    m_fadeOverlay->setGeometry(rect());
    m_fadeOverlay->raise();
    m_fadeOverlay->show();

    auto *anim = new QPropertyAnimation(effect, "opacity");
    anim->setDuration(durationMs);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    anim->setEasingCurve(QEasingCurve::OutQuad);

    connect(anim, &QPropertyAnimation::finished, this, [this, onDone, anim, effect]() {
        m_fadeOverlay->hide();
        if (onDone) onDone();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// 关卡切换 、 音乐同步
void MainWindow::onLevelChanged(int level)
{
    m_musicManager->playLevelMusic(level);
}

// 成就解锁 、 弹出 Toast
void MainWindow::onAchievementUnlocked(const AchievementInfo &info)
{
    AchievementToast::showToast(this, info);
}
