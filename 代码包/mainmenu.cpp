#include "mainmenu.h"
#include "savemanager.h"
#include "achievement.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QFont>
#include <QRadialGradient>
#include <QtMath>
#include <QFontMetrics>
#include <QKeyEvent>

// 调色板
static MenuPalette makePalette(bool postGame)
{
    if (postGame) {
        return {
            QColor("#ffd700"),                            // title
            {QColor("#ffd700"), QColor("#dab800"), QColor("#b89600")}, // btn[3]
            QColor("#ffeb3b"),                            // btnHover
            QColor("#daa520"),                            // unlocked
            QColor("#ffeb3b"),                            // completed
            QColor("#8b6914"),                            // accent
            QColor("#665100"),                            // accentDeep
            QColor(47, 35, 15, 60),                       // grid
            QColor(210, 170, 0, 145),                     // towerLine
            QColor(140, 100, 0, 20),                      // towerFill
            QColor(255, 215, 0),                          // artifactText
            QColor(120, 100, 40),                         // terminalText
            QColor(255, 215, 0, 185),                     // lakeText
            QColor("#ffd700"),                            // memoryTitle
            QColor("#8b6914"),                            // memoryHint
        };
    }
    return {
        QColor("#00ff88"),                                // title
        {QColor("#00ff88"), QColor("#00cc77"), QColor("#00aa66")}, // btn[3]
        QColor("#33ffaa"),                                // btnHover
        QColor("#00dd77"),                                // unlocked
        QColor("#33ffaa"),                                // completed
        QColor("#008844"),                                // accent
        QColor("#006633"),                                // accentDeep
        QColor(15, 47, 15, 60),                           // grid
        QColor(0, 210, 110, 145),                         // towerLine
        QColor(0, 140, 60, 20),                           // towerFill
        QColor(0, 200, 255),                              // artifactText
        QColor(51, 102, 68),                              // terminalText
        QColor(0, 255, 136, 185),                         // lakeText
        QColor("#00ff88"),                                // memoryTitle
        QColor("#008844"),                                // memoryHint
    };
}

void MainMenu::refreshPalette()
{
    m_isPostGame = SaveManager::exists("slot12");
    m_pal = makePalette(m_isPostGame);
}

// 按钮样式表生成器
static QString makeBtnStyleSheet(const QColor &base) {
    return QString(
        "QPushButton {"
        "  color: %1;"
        "  background: #001a0d;"
        "  border: 2px solid %1;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background: #002a15;"
        "  border: 2px solid #33ffaa;"
        "  color: #33ffaa;"
        "}"
        "QPushButton:pressed {"
        "  background: #003a1d;"
        "}"
    ).arg(base.name());
}

MainMenu::MainMenu(QWidget *parent) : QWidget(parent)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
    setFocusPolicy(Qt::StrongFocus);

    // 创建按钮
    struct BtnDef {
        QString text;
        QPushButton **member;
        QColor color;
        std::function<void()> action;
    };
    const BtnDef btns[] = {
        {QStringLiteral("开始游戏"), &m_startBtn,        QColor("#00ff88"), [this]{ emit startGameClicked(); }},
        {QStringLiteral("查看回忆"), &m_memoryBtn,       QColor("#00cc77"), [this]{ if (m_menuState==MenuState::Main) enterMemoryView(); }},
        {QStringLiteral("查看成就"), &m_achievementsBtn, QColor("#00aa66"), [this]{ if (m_menuState==MenuState::Main) enterAchievementsView(); }},
        {QStringLiteral("游戏设置"), &m_settingsBtn,     QColor("#00aa66"), [this]{ if (m_menuState==MenuState::Main) enterSettingsVolume(); }},
        {QStringLiteral("退出游戏"), &m_exitBtn,         QColor("#00aa66"), []{ QApplication::quit(); }},
    };
    for (const auto &b : btns) {
        auto *btn = new QPushButton(b.text, this);
        *b.member = btn;
        QFont f(QStringLiteral("SimHei"), 26);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 12);
        btn->setFont(f);
        btn->setStyleSheet(makeBtnStyleSheet(b.color));
        btn->raise();
        connect(btn, &QPushButton::clicked, this, b.action);
    }

    // —— Konami 码闪烁定时器 ——
    m_konamiFlashTimer = new QTimer(this);
    m_konamiFlashTimer->setInterval(80);
    connect(m_konamiFlashTimer, &QTimer::timeout, this, [this]() {
        m_konamiFlashTick++;
        update();
        if (m_konamiFlashTick > 38) {  // ~3 秒
            m_konamiFlashTimer->stop();
            m_konamiFlashTick = 0;
            update();
        }
    });

    // —— 闪烁光标 ——
    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(530);
    connect(m_blinkTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;
        update();
    });
    m_blinkTimer->start();
}

//  布局
void MainMenu::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int w = width();
    int h = height();

    const int count = 5;
    int btnW = 380, btnH = 72;
    int gap = 20;

    // 按钮组排布在「介绍文字下方」到「屏幕底部安全边距」之间。
    // 默认起点为中线偏下；若空间不足则等比缩小按钮高度与间距，
    // 始终把整组放进可用空间内，既不出界也不与上方文字重叠。
    const int startY = h / 2 + 30;
    const int bottomMargin = 40;            // 与屏幕下边界保留的安全间距
    const int avail = h - bottomMargin - startY;   // 可用纵向高度
    const int needed = count * btnH + (count - 1) * gap;

    double fontScale = 1.0;
    if (needed > avail && avail > 0) {
        double scale = static_cast<double>(avail) / needed;
        btnH = qMax(40, static_cast<int>(btnH * scale));
        gap  = qMax(8,  static_cast<int>(gap  * scale));
        fontScale = scale;
    }

    // 字号随按钮高度联动，避免压缩后文字溢出按钮
    QPushButton *btns[count] = {
        m_startBtn, m_memoryBtn, m_achievementsBtn, m_settingsBtn, m_exitBtn
    };
    QFont f(QStringLiteral("SimHei"), qMax(16, static_cast<int>(26 * fontScale)));
    f.setLetterSpacing(QFont::AbsoluteSpacing, 12 * fontScale);

    const int x = (w - btnW) / 2;
    for (int i = 0; i < count; ++i) {
        btns[i]->setFont(f);
        btns[i]->setFixedSize(btnW, btnH);
        btns[i]->move(x, startY + (btnH + gap) * i);
        btns[i]->raise();
    }
}

//  焦点
void MainMenu::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshPalette();
    updateButtonStyles();  // 按钮样式随调色板刷新
    setFocus();
}

//  键盘输入
void MainMenu::keyPressEvent(QKeyEvent *event)
{
    // —— Konami 码检测 ——
    static const QList<int> konamiPattern = {
        Qt::Key_Up, Qt::Key_Up, Qt::Key_Down, Qt::Key_Down,
        Qt::Key_Left, Qt::Key_Right, Qt::Key_Left, Qt::Key_Right,
        Qt::Key_B, Qt::Key_A
    };

    m_keyBuffer.append(event->key());
    if (m_keyBuffer.size() > konamiPattern.size())
        m_keyBuffer.removeFirst();

    if (!m_konamiActivated && m_keyBuffer == konamiPattern) {
        m_konamiActivated = true;
        SaveManager::saveProgress(12);
        // 顺便解锁全部成就
        const auto allAch = AchievementManager::instance()->allAchievements();
        for (const auto &info : allAch)
            AchievementManager::instance()->unlock(info.id);
        m_konamiFlashTick = 1;
        m_konamiFlashTimer->start();
        refreshMemoryEntries();
        update();
        return;
    }

    // —— 状态相关按键 ——
    if (m_menuState == MenuState::Main) {
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W) {
            m_keyboardSelection = (m_keyboardSelection - 1 + 5) % 5;
            updateButtonStyles();
            update();
        } else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S) {
            m_keyboardSelection = (m_keyboardSelection + 1) % 5;
            updateButtonStyles();
            update();
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter
                   || event->key() == Qt::Key_Space) {
            activateCurrentSelection();
        }
    } else if (m_menuState == MenuState::MemorySelect) {
        const int COLS = 2;
        const int TOTAL = m_memoryEntries.size();
        if (event->key() == Qt::Key_Escape) {
            resetToMainState();
            update();
        } else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W) {
            // 向上：减一列（index - COLS），跳过锁定项
            for (int t = m_memorySelection - COLS; t >= 0; t -= COLS) {
                if (m_memoryEntries[t].unlocked) {
                    m_memorySelection = t;
                    break;
                }
            }
            update();
        } else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S) {
            for (int t = m_memorySelection + COLS; t < TOTAL; t += COLS) {
                if (m_memoryEntries[t].unlocked) {
                    m_memorySelection = t;
                    break;
                }
            }
            update();
        } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
            // 向左：同一行内 index - 1，跳过锁定项
            int rowStart = (m_memorySelection / COLS) * COLS;
            for (int t = m_memorySelection - 1; t >= rowStart; --t) {
                if (m_memoryEntries[t].unlocked) {
                    m_memorySelection = t;
                    break;
                }
            }
            update();
        } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
            int rowEnd = ((m_memorySelection / COLS) + 1) * COLS;
            for (int t = m_memorySelection + 1; t < rowEnd && t < TOTAL; ++t) {
                if (m_memoryEntries[t].unlocked) {
                    m_memorySelection = t;
                    break;
                }
            }
            update();
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (!m_memoryEntries.isEmpty() && m_memoryEntries[m_memorySelection].unlocked) {
                int lvl = m_memoryEntries[m_memorySelection].level;
                emit loadLevelRequested(lvl);
            }
        }
    } else if (m_menuState == MenuState::Achievements) {
        if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) {
            resetToMainState();
            update();
        }
    } else if (m_menuState == MenuState::SettingsVolume) {
        if (event->key() == Qt::Key_Escape) {
            resetToMainState();
            update();
        } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
            m_volumePercent = qMax(0, m_volumePercent - 5);
            emit volumeChanged(m_volumePercent);
            update();
        } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
            m_volumePercent = qMin(100, m_volumePercent + 5);
            emit volumeChanged(m_volumePercent);
            update();
        } else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S) {
            m_volumePercent = qMax(0, m_volumePercent - 5);
            emit volumeChanged(m_volumePercent);
            update();
        } else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W) {
            m_volumePercent = qMin(100, m_volumePercent + 5);
            emit volumeChanged(m_volumePercent);
            update();
        }
    }
}

//  导航方法
void MainMenu::resetToMainState()
{
    refreshPalette();
    m_menuState = MenuState::Main;
    m_keyboardSelection = 0;
    m_memorySelection = 0;
    m_startBtn->setVisible(true);
    m_memoryBtn->setVisible(true);
    m_achievementsBtn->setVisible(true);
    m_settingsBtn->setVisible(true);
    m_exitBtn->setVisible(true);
    updateButtonStyles();
    setFocus();
}

void MainMenu::enterMemoryView()
{
    refreshMemoryEntries();
    m_menuState = MenuState::MemorySelect;
    // 选第一个 unlocked 的关卡
    m_memorySelection = 0;
    for (int i = 0; i < m_memoryEntries.size(); ++i) {
        if (m_memoryEntries[i].unlocked) {
            m_memorySelection = i;
            break;
        }
    }
    m_startBtn->setVisible(false);
    m_memoryBtn->setVisible(false);
    m_achievementsBtn->setVisible(false);
    m_settingsBtn->setVisible(false);
    m_exitBtn->setVisible(false);
    setFocus();
    update();
}

void MainMenu::enterAchievementsView()
{
    m_menuState = MenuState::Achievements;
    m_startBtn->setVisible(false);
    m_memoryBtn->setVisible(false);
    m_achievementsBtn->setVisible(false);
    m_settingsBtn->setVisible(false);
    m_exitBtn->setVisible(false);
    setFocus();
    update();
}

void MainMenu::enterSettingsVolume()
{
    m_menuState = MenuState::SettingsVolume;
    m_startBtn->setVisible(false);
    m_memoryBtn->setVisible(false);
    m_achievementsBtn->setVisible(false);
    m_settingsBtn->setVisible(false);
    m_exitBtn->setVisible(false);
    setFocus();
    update();
}

void MainMenu::activateCurrentSelection()
{
    switch (m_keyboardSelection) {
    case 0: emit startGameClicked();  break;
    case 1: enterMemoryView();        break;
    case 2: enterAchievementsView();  break;
    case 3: enterSettingsVolume();    break;
    case 4: QApplication::quit();     break;
    }
}

void MainMenu::updateButtonStyles()
{
    const QString selColor   = m_pal.btnHover.name();
    const QString selHov     = m_isPostGame ? "#ffef60" : "#55ffcc";
    const QString selBg      = m_isPostGame ? "#2a2000" : "#002a15";
    const QString unselBg    = m_isPostGame ? "#1a1000" : "#001a0d";
    const QString hoverBg    = m_isPostGame ? "#302800" : "#003a1d";
    const QString pressedBg  = m_isPostGame ? "#403000" : "#004a25";

    auto makeStyle = [&](const QString &color, bool selected) -> QString {
        QString baseColor   = selected ? selColor : color;
        QString bg          = selected ? selBg : unselBg;
        QString borderColor = selected ? selColor : color;
        QString hoverBorder = selected ? selHov : selColor;
        return QString(
            "QPushButton {"
            "  color: %1;"
            "  background: %2;"
            "  border: 2px solid %3;"
            "  border-radius: 6px;"
            "}"
            "QPushButton:hover {"
            "  background: %4;"
            "  border: 2px solid %5;"
            "  color: %5;"
            "}"
            "QPushButton:pressed {"
            "  background: %6;"
            "}"
        ).arg(baseColor, bg, borderColor, hoverBg, hoverBorder, pressedBg);
    };

    // 更新各按钮样式
    QPushButton *btns[] = {
        m_startBtn, m_memoryBtn, m_achievementsBtn, m_settingsBtn, m_exitBtn
    };
    for (int i = 0; i < 5; ++i) {
        int colorIdx = (i == 0) ? 0 : (i == 1) ? 1 : 2;
        btns[i]->setStyleSheet(makeStyle(m_pal.btn[colorIdx].name(), m_keyboardSelection == i));
    }
}

//  回忆数据
void MainMenu::refreshMemoryEntries()
{
    m_memoryEntries.clear();
    int highest = SaveManager::loadProgress();
    QStringList names = {
        "第一关：离开宿舍", "第二关：加速时间", "第三关：偷换概念",
                         "第四关：世界观重塑中", "第五关：湖边垂钓", "第六关：我进塔！",
                         "第七关：草坪演练", "第八关：初探机房",
                         "第九关：直面恶龙", "第十关：终焉之战",
                         "第十一关：尘埃落定", "第十二关：梦醒时分"
    };
    for (int i = 0; i < 12; ++i) {
        int lvl = i + 1;
        bool unlocked = (lvl <= highest);
        bool completed = SaveManager::exists("slot" + QString::number(lvl));
        m_memoryEntries.append({names[i], lvl, unlocked, completed});
    }
}

//  主绘制
void MainMenu::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::black);

    // 所有状态共享背景
    drawGrid(p);
    drawBoyaTower(p);
    drawDragonOnTower(p);
    drawArtifacts(p);
    drawWeimingLake(p);

    if (m_menuState == MenuState::Main) {
        drawTitle(p);
        drawIntro(p);
        drawTerminal(p);

        // 选中按钮左侧 ▶ 指示器
        QPushButton *targetBtn = nullptr;
        switch (m_keyboardSelection) {
        case 0: targetBtn = m_startBtn;         break;
        case 1: targetBtn = m_memoryBtn;        break;
        case 2: targetBtn = m_achievementsBtn;  break;
        case 3: targetBtn = m_settingsBtn;      break;
        case 4: targetBtn = m_exitBtn;          break;
        }
        if (targetBtn) {
            QFont arrFont("SimHei", 22);
            p.setFont(arrFont);
            p.setPen(m_pal.title);
            QRect r = targetBtn->geometry();
            p.drawText(r.left() - 30, r.center().y() + 8, "▶");
        }

        // 闪烁光标（在选中按钮右侧）
        if (m_cursorVisible) {
            QFont curFont("SimHei");
            curFont.setPixelSize(32);
            p.setFont(curFont);
            p.setPen(m_pal.title);
            QRect btnGeo = targetBtn ? targetBtn->geometry() : m_startBtn->geometry();
            p.drawText(btnGeo.right() + 10, btnGeo.center().y() + 11, "█");
        }
    } else if (m_menuState == MenuState::MemorySelect) {
        drawMemoryView(p);
    } else if (m_menuState == MenuState::Achievements) {
        drawAchievementsView(p);
    } else if (m_menuState == MenuState::SettingsVolume) {
        drawSettingsVolume(p);
    }

    drawScanlines(p);
    drawVignette(p);

    // Konami 闪烁效果
    if (m_konamiActivated && m_konamiFlashTick > 0 && m_konamiFlashTick <= 38) {
        bool on = (m_konamiFlashTick / 4) % 2 == 0;
        if (on && m_memoryBtn->isVisible()) {
            p.setPen(QPen(QColor(255, 255, 80, 200), 3));
            p.setBrush(Qt::NoBrush);
            QRect flashRect = m_memoryBtn->geometry().adjusted(-6, -6, 6, 6);
            p.drawRoundedRect(flashRect, 10, 10);
        }
    }
}

//  回忆视图绘制
void MainMenu::drawMemoryView(QPainter &p)
{
    int w = width();
    int h = height();

    // 半透明遮罩
    p.fillRect(rect(), QColor(0, 0, 0, 190));

    // 标题
    QFont titleFont("黑体");
    titleFont.setPixelSize(48);
    p.setFont(titleFont);
    p.setPen(m_pal.memoryTitle);
    p.drawText(QRect(0, 20, w, 60), Qt::AlignCenter, "· 查看回忆 ·");

    // 副标题
    QFont subFont("黑体");
    subFont.setPixelSize(18);
    p.setFont(subFont);
    p.setPen(m_pal.memoryHint);
    p.drawText(QRect(0, 82, w, 26), Qt::AlignCenter,
               "方向键选择   Enter 进入   Esc 返回");

    // 卡片网格：2 列 × 6 行
    const int cols = 2;
    const int cardW = 330;
    const int cardH = 130;
    const int gapX = 30;
    const int gapY = 20;

    int totalW = cols * cardW + (cols - 1) * gapX;
    int startX = (w - totalW) / 2;
    int startY = 110;

    for (int i = 0; i < m_memoryEntries.size(); ++i) {
        const auto &entry = m_memoryEntries[i];
        int col = i % cols;
        int row = i / cols;
        int cx = startX + col * (cardW + gapX);
        int cy = startY + row * (cardH + gapY);

        QRect cardRect(cx, cy, cardW, cardH);
        bool selected = (i == m_memorySelection);

        // 卡片背景与边框
        if (selected) {
            p.setPen(QPen(m_pal.title, 3.5));
            p.setBrush(QColor(0, 40, 30, 240));
        } else if (entry.unlocked) {
            p.setPen(QPen(m_pal.accent, 2.0));
            p.setBrush(QColor(15, 20, 18, 220));
        } else {
            p.setPen(QPen(QColor(50, 50, 50), 2.0));
            p.setBrush(QColor(15, 15, 18, 220));
        }
        p.drawRoundedRect(cardRect, 10, 10);

        // 选中标记（左上角三角箭头）
        if (selected) {
            QFont arrFont("黑体");
            arrFont.setPixelSize(22);
            p.setFont(arrFont);
            p.setPen(m_pal.title);
            p.drawText(cardRect.adjusted(14, 10, -14, 0), Qt::AlignLeft | Qt::AlignTop,
                       "▶");
        }

        // 关卡编号
        QFont numFont("黑体");
        numFont.setPixelSize(16);
        p.setFont(numFont);
        if (entry.unlocked) {
            p.setPen(m_pal.unlocked);
        } else {
            p.setPen(QColor(100, 100, 100));
        }
        int numOffset = selected ? 38 : 0;  // 选中时给箭头让位
        p.drawText(cardRect.adjusted(14 + numOffset, 14, -14, 0),
                   Qt::AlignLeft | Qt::AlignTop,
                   QStringLiteral("第%1关").arg(entry.level));

        // 关卡名称（大字）
        QFont nameFont("黑体");
        nameFont.setPixelSize(24);
        nameFont.setBold(selected);
        p.setFont(nameFont);
        if (entry.unlocked)
            p.setPen(selected ? m_pal.btnHover : m_pal.unlocked);
        else
            p.setPen(QColor(80, 80, 80));
        p.drawText(cardRect.adjusted(16, 40, -16, 0), Qt::AlignLeft | Qt::AlignTop,
                   entry.name);

        // 状态标签（右下角）
        QFont statusFont("黑体");
        statusFont.setPixelSize(15);
        p.setFont(statusFont);
        if (!entry.unlocked) {
            p.setPen(QColor(120, 120, 120));
            p.drawText(cardRect.adjusted(0, 0, -16, -12),
                       Qt::AlignRight | Qt::AlignBottom,
                       QStringLiteral("🔒 未解锁"));
        } else if (entry.completed) {
            p.setPen(m_pal.completed);
            p.drawText(cardRect.adjusted(0, 0, -16, -12),
                       Qt::AlignRight | Qt::AlignBottom,
                       QStringLiteral("★ 已通关"));
        } else {
            p.setPen(m_pal.unlocked);
            p.drawText(cardRect.adjusted(0, 0, -16, -12),
                       Qt::AlignRight | Qt::AlignBottom,
                       QStringLiteral("可进入"));
        }
    }

    // 底部提示
    QFont backFont("黑体");
    backFont.setPixelSize(16);
    p.setFont(backFont);
    p.setPen(m_pal.accentDeep);
    p.drawText(QRect(0, h - 46, w, 24), Qt::AlignCenter, "按 Esc 返回主菜单");
}

//  成就画廊绘制
void MainMenu::drawAchievementsView(QPainter &p)
{
    // 标题
    QFont titleFont("黑体");
    titleFont.setPixelSize(48);
    p.setFont(titleFont);
    p.setPen(m_pal.title);
    p.drawText(QRect(0, 20, width(), 60), Qt::AlignCenter, QStringLiteral("· 成 就 ·"));

    // 底部提示
    QFont hintFont("黑体");
    hintFont.setPixelSize(20);
    p.setFont(hintFont);
    p.setPen(m_pal.memoryHint);
    p.drawText(QRect(0, height() - 46, width(), 30), Qt::AlignCenter,
               QStringLiteral("按 ESC / Enter 返回主菜单"));

    QList<AchievementInfo> achievements = AchievementManager::instance()->allAchievements();

    // 布局：2 列，充分利用屏幕空间
    const int cols = 2;
    const int cardW = 330;
    const int cardH = 130;
    const int gapX = 30;
    const int gapY = 20;

    int totalW = cols * cardW + (cols - 1) * gapX;
    int startX = (width() - totalW) / 2;
    int startY = 100;

    for (int i = 0; i < achievements.size(); ++i) {
        const AchievementInfo &ach = achievements[i];
        int col = i % cols;
        int row = i / cols;
        int cx = startX + col * (cardW + gapX);
        int cy = startY + row * (cardH + gapY);

        QRect cardRect(cx, cy, cardW, cardH);
        bool unlocked = ach.unlocked;

        if (unlocked) {
            // 已解锁卡片：金色边框 + 半透明暗色背景
            p.setPen(QPen(m_pal.title, 3.0));
            p.setBrush(QColor(20, 25, 20, 240));
        } else {
            // 未解锁卡片：灰色边框 + 更暗的背景
            p.setPen(QPen(QColor(70, 70, 70), 2.5));
            p.setBrush(QColor(15, 15, 20, 230));
        }
        p.drawRoundedRect(cardRect, 12, 12);

        // 关卡标签（左上角）
        QFont lvlFont("黑体");
        lvlFont.setPixelSize(16);
        p.setFont(lvlFont);
        if (unlocked)
            p.setPen(QColor(180, 150, 80));
        else
            p.setPen(QColor(100, 100, 100));
        p.drawText(cardRect.adjusted(16, 12, -16, 0), Qt::AlignLeft | Qt::AlignTop,
                   QStringLiteral("第%1关").arg(ach.level));

        if (unlocked) {
            // 成就名
            QFont nameFont("黑体");
            nameFont.setPixelSize(24);
            nameFont.setBold(true);
            p.setFont(nameFont);
            p.setPen(m_pal.title);
            p.drawText(cardRect.adjusted(16, 36, -16, 0), Qt::AlignLeft | Qt::AlignTop,
                       ach.name);

            // 描述文字
            QFont descFont("黑体");
            descFont.setPixelSize(16);
            p.setFont(descFont);
            p.setPen(QColor(200, 200, 200));
            QRect descRect = cardRect.adjusted(16, 66, -16, -12);
            p.drawText(descRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                       ach.description);

            // 右下角 ★ 标记
            QFont starFont("黑体");
            starFont.setPixelSize(18);
            p.setFont(starFont);
            p.setPen(QColor(255, 215, 0));
            p.drawText(cardRect.adjusted(0, 0, -16, -12), Qt::AlignRight | Qt::AlignBottom,
                       QStringLiteral("★"));
        } else {
            // 未解锁：锁定图标居中偏上
            QFont lockIconFont("黑体");
            lockIconFont.setPixelSize(38);
            p.setFont(lockIconFont);
            p.setPen(QColor(110, 110, 110));
            p.drawText(QRect(cx, cy + 22, cardW, 48), Qt::AlignCenter,
                       QStringLiteral("🔒"));

            // 未解锁提示
            QFont lockFont("黑体");
            lockFont.setPixelSize(17);
            p.setFont(lockFont);
            p.setPen(QColor(140, 140, 140));
            QRect lockRect = cardRect.adjusted(16, 72, -16, -12);
            p.drawText(lockRect, Qt::AlignHCenter | Qt::AlignTop,
                       QStringLiteral("在第%1关中解锁").arg(ach.level));
        }
    }
}

//  音量设置绘制
void MainMenu::drawSettingsVolume(QPainter &p)
{
    p.fillRect(rect(), QColor(0, 0, 0, 200));

    int w = width();
    int cy = height() / 2;

    // 标题
    QFont titleFont("SimHei", 28);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 8);
    p.setFont(titleFont);
    p.setPen(m_pal.title);
    p.drawText(QRect(0, cy - 100, w, 50), Qt::AlignCenter, "游戏设置");

    // 副标题
    QFont subFont("SimHei", 18);
    p.setFont(subFont);
    p.setPen(m_pal.unlocked);
    p.drawText(QRect(0, cy - 50, w, 30), Qt::AlignCenter, "背景音乐音量");

    // 百分比数字
    QFont pctFont("SimHei", 36);
    pctFont.setBold(true);
    p.setFont(pctFont);
    p.setPen(m_pal.title);
    p.drawText(QRect(0, cy - 10, w, 44), Qt::AlignCenter,
               QString::number(m_volumePercent) + "%");

    // 音量条：600x24 圆角矩形
    const int barW = 600;
    const int barH = 24;
    const int barX = (w - barW) / 2;
    const int barY = cy + 46;

    // 背景
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(40, 40, 40));
    p.drawRoundedRect(barX, barY, barW, barH, 6, 6);

    // 已填充部分
    int fillW = barW * m_volumePercent / 100;
    if (fillW > 0) {
        p.setBrush(m_pal.title);
        p.drawRoundedRect(barX, barY, fillW, barH, 6, 6);
    }

    // 边框
    p.setPen(QPen(m_pal.accent, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(barX, barY, barW, barH, 6, 6);

    // 刻度标记（每 25% 一条竖线）
    QFont tickFont("SimHei", 11);
    p.setFont(tickFont);
    p.setPen(m_pal.accent);
    for (int pct = 0; pct <= 100; pct += 25) {
        int tx = barX + barW * pct / 100;
        p.drawLine(tx, barY - 6, tx, barY + barH + 6);
        QString label = QString::number(pct) + "%";
        p.drawText(QRect(tx - 20, barY + barH + 10, 40, 16),
                   Qt::AlignCenter, label);
    }

    // 底部提示
    QFont hintFont("SimHei", 14);
    p.setFont(hintFont);
    p.setPen(m_pal.accent);
    p.drawText(QRect(0, cy + 120, w, 24),
               Qt::AlignCenter, "← 、 或 A D 调整音量    Esc 返回");
}

//  背景绘制
void MainMenu::drawGrid(QPainter &p)
{
    int w = width();
    int h = height();
    p.setPen(QPen(m_pal.grid, 0.4));
    for (int x = 0; x < w; x += 40)
        p.drawLine(x, 0, x, h);
    for (int y = 0; y < h; y += 40)
        p.drawLine(0, y, w, y);
}

void MainMenu::drawBoyaTower(QPainter &p)
{
    int winW = width();
    int winH = height();

    const double towerCenterX = winW - winW / 12.0;
    const double baseHalfW = winW / 10.0;
    const double baseY = winH + 14;

    QColor lineC = m_pal.towerLine;
    QColor fillC = m_pal.towerFill;
    QColor eaveC = m_pal.towerLine;
    eaveC.setAlpha(100);

    p.setBrush(fillC);

    double curY = baseY;
    double curHalfW = baseHalfW;
    int levelCount = 0;
    const double layerH = 14.0;
    const double shrink = 0.45;

    while (curY > -20) {
        QPainterPath body;
        body.moveTo(towerCenterX - curHalfW, curY);
        body.lineTo(towerCenterX - curHalfW * 0.90, curY - layerH);
        body.lineTo(towerCenterX + curHalfW * 0.90, curY - layerH);
        body.lineTo(towerCenterX + curHalfW, curY);
        body.closeSubpath();
        p.fillPath(body, fillC);
        p.setPen(QPen(lineC, 0.6));
        p.drawPath(body);

        double eave = 7.0;
        p.setPen(QPen(eaveC, 1.2));
        p.drawLine(QPointF(towerCenterX - curHalfW - eave, curY),
                   QPointF(towerCenterX + curHalfW, curY));
        p.drawLine(QPointF(towerCenterX - curHalfW - eave, curY),
                   QPointF(towerCenterX - curHalfW - eave - 3, curY - 4));

        curY -= layerH;
        curHalfW -= shrink;
        ++levelCount;

        if (levelCount == 2) {
            p.setPen(QPen(lineC, 1.4));
            p.drawLine(QPointF(towerCenterX, curY + 2),
                       QPointF(towerCenterX, curY + layerH + 8));
            p.setPen(QPen(eaveC, 2.0));
            p.drawLine(QPointF(towerCenterX - curHalfW - 10, curY + layerH),
                       QPointF(towerCenterX + curHalfW, curY + layerH));
        }
    }

    QFont towerFont("SimHei");
    towerFont.setPixelSize(28);
    p.setFont(towerFont);
    QColor towerLabelColor = m_pal.towerLine;
    towerLabelColor.setAlpha(170);
    p.setPen(towerLabelColor);
    p.drawText(QRectF(towerCenterX - baseHalfW, winH - 80,
                      baseHalfW + 40, 40),
               Qt::AlignLeft | Qt::AlignVCenter, "博 雅 塔");
}

void MainMenu::drawDragonOnTower(QPainter &p)
{
    int winW = width();
    int winH = height();

    const double towerCenterX = winW - winW / 12.0;
    const double baseHalfW = winW / 10.0;
    const double leftEdge = towerCenterX - baseHalfW;
    QFont font("SimSun");

    font.setPixelSize(80);
    p.setFont(font);
    double headX = leftEdge - 80;
    double headY = winH * 0.20;
    p.setPen(QColor(255, 20, 20, 225));
    p.drawText(static_cast<int>(headX), static_cast<int>(headY), "龙");

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 200, 200));
    p.drawEllipse(QPointF(headX + 52, headY + 20), 5.5, 5.5);

    font.setPixelSize(22);
    p.setFont(font);
    struct Seg { double hRatio; double xOffset; int sz; int a; };
    const QList<Seg> body = {
        {0.27, -46, 48, 190},
        {0.33, -36, 44, 175},
        {0.39, -30, 40, 165},
        {0.46,  20, 36, 155},
        {0.53, -26, 34, 145},
        {0.61,  18, 32, 135},
        {0.69, -22, 30, 125},
        {0.77,  14, 28, 115},
        {0.85, -16, 26, 105},
    };
    for (const auto &s : body) {
        font.setPixelSize(s.sz);
        p.setFont(font);
        p.setPen(QColor(220, 18, 18, s.a));
        p.drawText(static_cast<int>(leftEdge + s.xOffset),
                   static_cast<int>(winH * (1.0 - s.hRatio)), "龙");
    }

    font.setPixelSize(34);
    p.setFont(font);
    p.setPen(QColor(255, 140, 0, 165));
    p.drawText(static_cast<int>(headX - 50), static_cast<int>(headY + 14), "焰");
    font.setPixelSize(24);
    p.setFont(font);
    p.drawText(static_cast<int>(headX - 76), static_cast<int>(headY), "焰");
    p.drawText(static_cast<int>(headX - 64), static_cast<int>(headY - 26), "焰");

    font.setPixelSize(28);
    p.setFont(font);
    p.setPen(QColor(200, 30, 30, 140));
    p.drawText(static_cast<int>(leftEdge - 16), static_cast<int>(headY - 50), "翼");
}

void MainMenu::drawArtifacts(QPainter &p)
{
    int winW = width();
    int winH = height();
    const double towerCenterX = winW - winW / 12.0;
    const double baseHalfW = winW / 10.0;

    QFont font("SimHei");
    font.setBold(true);

    font.setPixelSize(30);
    for (int i = 5; i >= 1; --i) {
        QColor glowC = m_pal.artifactText;
        glowC.setAlpha(10 + i * 10);
        p.setPen(glowC);
        p.setFont(font);
        p.drawText(QRect(40 - i, 44 - i, 400, 44),
                   Qt::AlignLeft | Qt::AlignVCenter, "⚔ 贝克思贝斯之剑");
    }
    QColor mainArtC = m_pal.artifactText;
    mainArtC.setAlpha(200);
    p.setPen(mainArtC);
    p.drawText(QRect(40, 44, 400, 44), Qt::AlignLeft | Qt::AlignVCenter, "⚔ 贝克思贝斯之剑");

    double shieldX = towerCenterX - baseHalfW - 120;
    double shieldY = winH - 130;
    font.setPixelSize(30);
    for (int i = 5; i >= 1; --i) {
        QColor glowC = m_pal.artifactText;
        glowC.setAlpha(10 + i * 10);
        p.setPen(glowC);
        p.setFont(font);
        p.drawText(QRectF(shieldX - i, shieldY - i, 280, 44),
                   Qt::AlignLeft | Qt::AlignVCenter, "🛡 恩特之盾");
    }
    p.setPen(mainArtC);
    p.drawText(QRectF(shieldX, shieldY, 280, 44),
               Qt::AlignLeft | Qt::AlignVCenter, "🛡 恩特之盾");
}

void MainMenu::drawTitle(QPainter &p)
{
    int w = width();
    int cy = height() / 2 - 180;
    QFont font("SimHei");
    font.setPixelSize(84);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 24);

    for (int i = 7; i >= 1; --i) {
        QColor glowC = m_pal.title;
        glowC.setAlpha(18 + i * 7);
        p.setPen(glowC);
        p.setFont(font);
        p.drawText(QRectF(0, cy - 90 - i, w, 90), Qt::AlignHCenter | Qt::AlignVCenter, "文 字 游 戏");
    }
    p.setPen(m_pal.title);
    p.drawText(QRectF(0, cy - 90, w, 90), Qt::AlignHCenter | Qt::AlignVCenter, "文 字 游 戏");

    QFont subFont("SimHei");
    subFont.setPixelSize(18);
    p.setFont(subFont);
    QColor subC = m_pal.accent;
    subC.setAlpha(160);
    p.setPen(subC);
    p.drawText(QRectF(0, cy + 6, w, 28), Qt::AlignHCenter | Qt::AlignVCenter, "—— 太 以 编 辑 ——");
}

void MainMenu::drawIntro(QPainter &p)
{
    int w = width();
    int cy = height() / 2 - 100;
    QFont font("SimHei");
    font.setPixelSize(22);
    p.setFont(font);

    QColor introC = m_pal.title;
    introC.setAlpha(225);
    p.setPen(introC);
    p.drawText(QRectF(0, cy + 24, w, 32), Qt::AlignHCenter | Qt::AlignVCenter,
               "你的每一次键入，都在改写这个世界的源代码。");

    p.setPen(QColor(255, 170, 0, 205));
    p.drawText(QRectF(0, cy + 62, w, 32), Qt::AlignHCenter | Qt::AlignVCenter,
               "恶龙已经苏醒。涌动的暗流正在侵蚀未名湖的堤岸。");
}

void MainMenu::drawWeimingLake(QPainter &p)
{
    int h = height();
    QFont font("SimSun");
    font.setPixelSize(38);
    p.setFont(font);

    for (int i = 3; i >= 1; --i) {
        QColor lakeC = m_pal.lakeText;
        lakeC.setAlpha(lakeC.alpha() * (30 + i * 16) / 185);
        p.setPen(lakeC);
        p.drawText(70 - i, h - 210 - i, "未 名 湖");
    }
    p.setPen(m_pal.lakeText);
    p.drawText(70, h - 210, "未 名 湖");
}

void MainMenu::drawTerminal(QPainter &p)
{
    int h = height();
    QFont font("Consolas");
    font.setPixelSize(18);
    p.setFont(font);

    int y = h - 90;
    QColor termC = m_pal.terminalText;
    termC.setAlpha(200);
    p.setPen(termC);
    p.drawText(70, y, "$ tarin_edit --campus=pku --mode=adventure");
    y += 28;
    QColor termC2 = m_pal.terminalText;
    termC2.setAlpha(200);
    p.setPen(termC2);
    p.drawText(70, y, "[OK] 博雅塔·未名湖·五四路——北大校园已加载。");
}

void MainMenu::drawScanlines(QPainter &p)
{
    int w = width();
    int h = height();
    p.setPen(Qt::NoPen);
    for (int y = 0; y < h; y += 4) {
        p.fillRect(0, y, w, 1, QColor(0, 0, 0, 28));
        p.fillRect(0, y + 2, w, 1, QColor(0, 0, 0, 14));
    }
}

void MainMenu::drawVignette(QPainter &p)
{
    int w = width();
    int h = height();
    QRadialGradient vignette(w / 2.0, h / 2.0, qMax(w, h) * 0.7);
    vignette.setColorAt(0.45, QColor(0, 0, 0, 0));
    vignette.setColorAt(1.0, QColor(0, 0, 0, 110));
    p.fillRect(rect(), vignette);
}
