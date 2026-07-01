#include "pausemenu.h"
#include "animutils.h"
#include "savemanager.h"

#include <QFont>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QPropertyAnimation>
#include <cmath>

PauseMenu::PauseMenu(QObject *parent)
    : QObject(parent)
{
}

// 调色板
void PauseMenu::initPalette()
{
    m_isPostGame = SaveManager::exists("slot12");
    if (m_isPostGame) {
        m_palTitle  = QColor("#ffd700");
        m_palText   = QColor("#ffeebb");
        m_palDim    = QColor("#6b5b00");
        m_palBorder = QColor("#daa520");
        m_palBg     = QColor(30, 25, 5, 215);
        m_palAccent = QColor("#ffeb3b");
    } else {
        m_palTitle  = QColor("#00ff88");
        m_palText   = QColor("#ccffdd");
        m_palDim    = QColor("#2a5530");
        m_palBorder = QColor("#00aa66");
        m_palBg     = QColor(5, 25, 12, 215);
        m_palAccent = QColor("#33ffaa");
    }
}

// 面板辅助
QGraphicsRectItem *PauseMenu::addPanel(qreal x, qreal y, qreal w, qreal h,
                                        const QColor &fill, const QColor &border,
                                        qreal z)
{
    auto *rect = new QGraphicsRectItem(x, y, w, h);
    rect->setBrush(QBrush(fill));
    rect->setPen(QPen(border, 1.5));
    rect->setZValue(z);
    m_scene->addItem(rect);
    m_panels.append(rect);
    return rect;
}

// 扫描线装饰
void PauseMenu::drawScanlineOverlay()
{
    QRectF r = m_scene->sceneRect();
    int h = static_cast<int>(r.height());
    for (int y = 0; y < h; y += 4) {
        auto *line1 = new QGraphicsRectItem(0, y, r.width(), 1);
        line1->setBrush(QBrush(m_palBg.darker(250)));
        line1->setPen(Qt::NoPen);
        line1->setZValue(999);
        line1->setOpacity(0.5);
        m_scene->addItem(line1);
        m_panels.append(line1);
        auto *line2 = new QGraphicsRectItem(0, y + 2, r.width(), 1);
        line2->setBrush(QBrush(QColor(0, 0, 0, 40)));
        line2->setPen(Qt::NoPen);
        line2->setZValue(999);
        m_scene->addItem(line2);
        m_panels.append(line2);
    }
}

// 光标摆放
void PauseMenu::placeCursorAtSelection()
{
    if (!m_cursor) return;

    if (m_state == State::MemorySelect) {
        QRectF sr = m_scene->sceneRect();
        const qreal cardW = 170, cardH = 64, gapX = 18, gapY = 14;
        qreal totalW = 4 * cardW + 3 * gapX;
        qreal startX = (sr.width() - totalW) / 2.0;
        qreal startY = 110;
        int selRow = m_selection / 4;
        int selCol = m_selection % 4;
        m_cursor->setPos(startX + selCol * (cardW + gapX) + 8,
                         startY + selRow * (cardH + gapY) + 18);
    } else if (m_state == State::ConfirmReturn) {
        QRectF sr = m_scene->sceneRect();
        qreal pw = 480, btnW = 140;
        qreal px = (sr.width() - pw) / 2.0;
        qreal py = (sr.height() - 220) / 2.0;
        qreal btnY = py + 120;
        qreal bx = px + (pw / 2.0 - btnW) / 2.0 + m_selection * (pw / 2.0);
        m_cursor->setPos(bx + 8, btnY + 8);
    } else {  // Main
        const qreal panelX = 290;
        const qreal startY = 200, spacing = 54;
        m_cursor->setPos(panelX + 16, startY + m_selection * spacing + 8);
    }
}

// 打开/关闭
void PauseMenu::open(QGraphicsTextItem *playerItem) {
    m_active = true;
    m_gamePlayer = playerItem;  // 仅记录引用，不修改其颜色
    initPalette();

    // 创建独立光标"我"
    if (!m_cursor && m_scene) {
        m_cursor = m_scene->addText("我", QFont("黑体", 24));
        m_cursor->setZValue(1000);
    }
    if (m_cursor)
        m_cursor->setDefaultTextColor(m_palTitle);

    buildMain();
}

void PauseMenu::close() {
    clearAll();
    // 销毁独立光标
    if (m_cursor) {
        if (m_cursor->scene()) m_scene->removeItem(m_cursor);
        delete m_cursor;
        m_cursor = nullptr;
    }
    m_active = false;
    m_state = State::Inactive;
    m_gamePlayer = nullptr;
}

// 按键处理
void PauseMenu::handleKeyPress(int key) {
    if (!m_active) return;

    if (key == Qt::Key_Escape) {
        if (m_state == State::Main) {
            emit resumeGame();
        } else {
            clearAll();
            buildMain();
        }
        return;
    }

    if (m_state == State::ShowHint) {
        clearAll();
        buildMain();
        return;
    }

    // WASD / 方向键
    if (key == Qt::Key_W || key == Qt::Key_Up)           navigate(0, -1);
    if (key == Qt::Key_S || key == Qt::Key_Down)         navigate(0, 1);
    if (key == Qt::Key_A || key == Qt::Key_Left)         navigate(-1, 0);
    if (key == Qt::Key_D || key == Qt::Key_Right)        navigate(1, 0);
    if (key == Qt::Key_Return || key == Qt::Key_Enter)   confirm();
}

// 主菜单
void PauseMenu::buildMain(int sel) {
    clearAll();
    m_state = State::Main;
    m_selection = (sel >= 0) ? sel : 0;
    m_optionCount = 5;

    QRectF sr = m_scene->sceneRect();

    // 暗色遮罩
    m_overlay = m_scene->addRect(sr, QPen(Qt::NoPen), QBrush(QColor(0, 0, 0, 170)));
    m_overlay->setZValue(998);

    // CRT 扫描线
    drawScanlineOverlay();

    // 标题
    m_title = m_scene->addText("—— 暂停 ——", QFont("黑体", 30, QFont::Bold));
    m_title->setDefaultTextColor(m_palTitle);
    m_title->setPos(280, 140);
    m_title->setZValue(1000);

    // 选项列表
    QStringList opts = {"继续旅程", "时间回溯", "返回主菜单", "寻求提示", "查看回忆"};
    const qreal panelX = 290, panelW = 260;
    const qreal startY = 200, spacing = 54, panelH = 42;

    for (int i = 0; i < opts.size(); ++i) {
        qreal py = startY + i * spacing;

        bool sel = (i == m_selection);
        QColor bg  = sel ? QColor(m_palBg.red(), m_palBg.green(), m_palBg.blue(), 240)
                         : QColor(0, 0, 0, 160);
        QColor bor = sel ? m_palTitle : m_palBorder;
        addPanel(panelX, py, panelW, panelH, bg, bor, 999);

        auto *item = m_scene->addText(opts[i], QFont("黑体", 20));
        item->setDefaultTextColor(sel ? m_palTitle : m_palText);
        item->setPos(panelX + 50, py + 6);
        item->setZValue(1000);
        m_items.append(item);
    }

    // 确保光标存在并加入场景
    if (!m_cursor && m_scene) {
        m_cursor = m_scene->addText("我", QFont("黑体", 24));
        m_cursor->setZValue(1000);
    } else if (m_cursor && !m_cursor->scene()) {
        m_scene->addItem(m_cursor);
    }
    if (m_cursor) {
        m_cursor->setDefaultTextColor(m_palTitle);
        placeCursorAtSelection();
    }

    updateSubtitle();
}

// 确认对话框
void PauseMenu::buildConfirm(const QString &msg, int sel) {
    clearAll();
    m_state = State::ConfirmReturn;
    m_selection = (sel >= 0) ? sel : 0;
    m_optionCount = 2;

    QRectF sr = m_scene->sceneRect();

    m_overlay = m_scene->addRect(sr, QPen(Qt::NoPen), QBrush(QColor(0, 0, 0, 180)));
    m_overlay->setZValue(998);
    drawScanlineOverlay();

    // 居中面板
    qreal pw = 480, ph = 220;
    qreal px = (sr.width() - pw) / 2.0;
    qreal py = (sr.height() - ph) / 2.0;
    addPanel(px, py, pw, ph, m_palBg, m_palBorder, 999);

    m_confirmText = m_scene->addText(msg, QFont("黑体", 20));
    m_confirmText->setDefaultTextColor(m_palText);
    m_confirmText->setPos(px + (pw - m_confirmText->boundingRect().width()) / 2.0, py + 40);
    m_confirmText->setZValue(1000);

    QStringList opts = {"确定", "取消"};
    qreal btnW = 140, btnH = 40;
    qreal btnY = py + 120;
    for (int i = 0; i < opts.size(); ++i) {
        qreal bx = px + (pw / 2.0 - btnW) / 2.0 + i * (pw / 2.0);
        bool sel = (i == m_selection);
        QColor bg  = sel ? QColor(m_palBg.red(), m_palBg.green(), m_palBg.blue(), 240)
                         : QColor(0, 0, 0, 160);
        QColor bor = sel ? m_palTitle : m_palBorder;
        addPanel(bx, btnY, btnW, btnH, bg, bor, 999);

        auto *item = m_scene->addText(opts[i], QFont("黑体", 20));
        item->setDefaultTextColor(sel ? m_palTitle : m_palText);
        item->setPos(bx + 40, btnY + 6);
        item->setZValue(1000);
        m_items.append(item);
    }

    if (!m_cursor && m_scene) {
        m_cursor = m_scene->addText("我", QFont("黑体", 24));
        m_cursor->setZValue(1000);
    } else if (m_cursor && !m_cursor->scene()) {
        m_scene->addItem(m_cursor);
    }
    if (m_cursor) {
        m_cursor->setDefaultTextColor(m_palTitle);
        placeCursorAtSelection();
    }
}

// 回忆选择（卡片网格）
void PauseMenu::buildMemorySelect(int sel) {
    clearAll();
    m_state = State::MemorySelect;

    QRectF sr = m_scene->sceneRect();

    m_overlay = m_scene->addRect(sr, QPen(Qt::NoPen), QBrush(QColor(0, 0, 0, 185)));
    m_overlay->setZValue(998);
    drawScanlineOverlay();

    // 标题
    m_title = m_scene->addText("—— 查看回忆 ——", QFont("黑体", 26, QFont::Bold));
    m_title->setDefaultTextColor(m_palTitle);
    m_title->setPos(270, 42);
    m_title->setZValue(1000);

    // 副标题
    auto *hint = m_scene->addText("WASD / 方向键 选择    Enter 进入    Esc 返回",
                                   QFont("黑体", 12));
    hint->setDefaultTextColor(m_palDim);
    hint->setPos(220, 78);
    hint->setZValue(1000);
    m_items.append(hint);

    // 卡片网格：4 列 × 3 行
    QStringList names = {"第一关：离开宿舍", "第二关：加速时间", "第三关：偷换概念",
                         "第四关：世界观重塑中", "第五关：湖边垂钓", "第六关：我进塔！",
                         "第七关：草坪演练", "第八关：初探机房",
                         "第九关：直面恶龙", "第十关：终焉之战",
                         "第十一关：尘埃落定", "第十二关：梦醒时分"};

    const int cols = 4;
    const qreal cardW = 170, cardH = 64;
    const qreal gapX = 18, gapY = 14;
    qreal totalW = cols * cardW + (cols - 1) * gapX;
    qreal startX = (sr.width() - totalW) / 2.0;
    qreal startY = 110;

    m_optionCount = names.size();
    int highest = SaveManager::loadProgress();

    // 首次进入时自动跳到第一个已解锁关卡
    if (sel < 0) {
        m_selection = 0;
        for (int i = 0; i < names.size(); ++i) {
            if (i + 1 <= highest) { m_selection = i; break; }
        }
    } else {
        m_selection = sel;
    }

    for (int i = 0; i < names.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        qreal cx = startX + col * (cardW + gapX);
        qreal cy = startY + row * (cardH + gapY);

        int lvl = i + 1;
        bool unlocked = (lvl <= highest);
        bool completed = SaveManager::exists("slot" + QString::number(lvl));
        bool selected = (i == m_selection);

        // 卡片背景
        QColor cardBg  = selected ? QColor(m_palBg.red(), m_palBg.green(), m_palBg.blue(), 245)
                                  : QColor(0, 0, 0, 170);
        QColor cardBor = selected ? m_palTitle
                                  : (unlocked ? m_palBorder : QColor(60, 60, 60));
        cardBor.setAlpha(selected ? 255 : (unlocked ? 180 : 100));
        addPanel(cx, cy, cardW, cardH, cardBg, cardBor, 999);

        // 选中指示
        if (selected) {
            auto *indicator = m_scene->addText("▶", QFont("黑体", 14));
            indicator->setDefaultTextColor(m_palTitle);
            indicator->setPos(cx + 6, cy + cardH - 22);
            indicator->setZValue(1000);
            m_items.append(indicator);
        }

        // 关卡名
        QString label;
        QColor labelColor;
        if (unlocked) {
            label = QString::number(lvl) + " · " + names[i].mid(names[i].indexOf("：") + 1);
            labelColor = selected ? m_palTitle : m_palText;
        } else {
            label = QString::number(lvl) + " · 🔒";
            labelColor = m_palDim;
        }
        auto *cardLabel = m_scene->addText(label, QFont("黑体", 13, selected ? QFont::Bold : QFont::Normal));
        cardLabel->setDefaultTextColor(labelColor);
        cardLabel->setPos(cx + 10, cy + 8);
        cardLabel->setZValue(1000);
        m_items.append(cardLabel);

        // 状态标记
        if (completed) {
            auto *tag = m_scene->addText("✓已通关", QFont("黑体", 10));
            tag->setDefaultTextColor(m_palAccent);
            tag->setPos(cx + cardW - 60, cy + cardH - 20);
            tag->setZValue(1000);
            m_items.append(tag);
        } else if (unlocked && !completed) {
            auto *tag = m_scene->addText("○进行中", QFont("黑体", 10));
            tag->setDefaultTextColor(QColor(m_palAccent.red(), m_palAccent.green(),
                                            m_palAccent.blue(), 120));
            tag->setPos(cx + cardW - 60, cy + cardH - 20);
            tag->setZValue(1000);
            m_items.append(tag);
        }
    }

    if (!m_cursor && m_scene) {
        m_cursor = m_scene->addText("我", QFont("黑体", 24));
        m_cursor->setZValue(1000);
    } else if (m_cursor && !m_cursor->scene()) {
        m_scene->addItem(m_cursor);
    }
    if (m_cursor) {
        m_cursor->setDefaultTextColor(m_palTitle);
        placeCursorAtSelection();
    }
}

// 导航
void PauseMenu::navigate(int dx, int dy) {
    if (m_state == State::MemorySelect) {
        // 卡片网格导航：上下切换行，左右切换列，跳过未解锁项
        int highest = SaveManager::loadProgress();
        int cols = 4;
        int start = m_selection;
        int newSel = m_selection;
        int row = newSel / cols;
        int col = newSel % cols;

        if (dx != 0) {
            // 水平移动
            int attempts = 0;
            do {
                col = (col + dx + cols) % cols;
                newSel = row * cols + col;
                attempts++;
            } while (newSel + 1 > highest && newSel != start && attempts < cols);
        } else if (dy != 0) {
            // 垂直移动
            int attempts = 0;
            do {
                row = (row + dy + 3) % 3;  // 3 rows
                newSel = row * cols + col;
                attempts++;
            } while (newSel + 1 > highest && newSel != start && attempts < 3);
        }

        m_selection = newSel;
    } else {
        // 线性菜单：W/S 上下，A/D 也当上下
        int d = (dx != 0) ? dx : dy;
        m_selection = (m_selection + d + m_optionCount) % m_optionCount;
    }

    updateSubtitle();

    // 更新光标位置 + 重建高亮
    if (m_cursor)
        placeCursorAtSelection();

    if (m_state == State::Main)
        buildMain(m_selection);
    else if (m_state == State::ConfirmReturn) {
        QString msg = (m_pendingAction == "reset")
            ? "你确定要复原当前关卡吗？" : "你确定要返回主菜单吗？";
        buildConfirm(msg, m_selection);
    } else if (m_state == State::MemorySelect)
        buildMemorySelect(m_selection);
}

// 确认
void PauseMenu::confirm() {
    if (m_state == State::Main) {
        switch (m_selection) {
        case 0:
            emit resumeGame();
            break;
        case 1:
            m_pendingAction = "reset";
            buildConfirm("你确定要复原当前关卡吗？");
            break;
        case 2:
            m_pendingAction = "menu";
            buildConfirm("你确定要返回主菜单吗？");
            break;
        case 3: {
            QStringList hints = {
                "第一关：把【没】推到【上锁的门】旁边",
                "第二关：把【八】推入【早上七点】替换掉【七】，或许推别的数字也有事情发生...？",
                "第三关：我一定要找到车？车一定要找到我！",
                "第四关：休息关，播放对话即可",
                "第五关：黄金矿工好像致敬过这一关",
                "第六关：哇，是少有的带物理引擎的关卡！跳就对了",
                "第七关：面朝目标，使用两件神器吧！",
                "第八关：按F键交互，删除“找不到”的“不”字",
                "第九关：考验反应力的时刻到了！用Enter来弹反吧，记得面朝目标！",
                "第十关：打飞机又是多少人的童年呢",
                "第十一关：结束了...吗？",
                "第十二关：故事之外，是更大的故事..."
            };
            int idx = qBound(0, m_currentLevel - 1, hints.size() - 1);
            clearAll();
            m_state = State::ShowHint;

            QRectF sr = m_scene->sceneRect();
            m_overlay = m_scene->addRect(sr, QPen(Qt::NoPen), QBrush(QColor(0, 0, 0, 180)));
            m_overlay->setZValue(998);
            drawScanlineOverlay();

            qreal pw = 560, ph = 200;
            qreal px = (sr.width() - pw) / 2.0;
            qreal py = (sr.height() - ph) / 2.0;
            addPanel(px, py, pw, ph, m_palBg, m_palBorder, 999);

            auto *tipTitle = m_scene->addText("💡 寻求提示", QFont("黑体", 18, QFont::Bold));
            tipTitle->setDefaultTextColor(m_palTitle);
            tipTitle->setPos(px + 16, py + 10);
            tipTitle->setZValue(1000);
            m_items.append(tipTitle);

            m_hintText = m_scene->addText(hints[idx], QFont("黑体", 16));
            m_hintText->setDefaultTextColor(m_palText);
            m_hintText->setTextWidth(pw - 32);
            m_hintText->setPos(px + 16, py + 44);
            m_hintText->setZValue(1000);

            auto *back = m_scene->addText("（按任意键返回）", QFont("黑体", 13));
            back->setDefaultTextColor(m_palDim);
            back->setPos(px + pw - 160, py + ph - 26);
            back->setZValue(1000);
            m_items.append(back);

            // 隐藏光标
            if (m_cursor) m_cursor->setVisible(false);
            break;
        }
        case 4:
            buildMemorySelect();
            break;
        }
    } else if (m_state == State::ConfirmReturn) {
        if (m_selection == 0) {
            if (m_pendingAction == "menu") {
                emit returnToMenuRequested();
            } else {
                emit resetLevelRequested();
            }
            m_pendingAction.clear();
        } else {
            m_pendingAction.clear();
            buildMain();
        }
    } else if (m_state == State::MemorySelect) {
        int lvl = m_selection + 1;
        if (lvl <= SaveManager::loadProgress())
            emit loadLevelRequested(lvl);
    }
}

// 副标题动画
void PauseMenu::updateSubtitle() {
    QStringList subs = {
        "继续踏上你不平凡的贵校旅程。",
        "我们的故事允许后悔药的存在。现实呢？",
        "稍作休息吧，有时需要停下来整理思绪。",
        "故事之外，有谁......还在？",
        "回望来时的路吧，然后收拾行囊再出发。"
    };

    int sel = m_selection;
    if (sel < 0 || sel >= subs.size()) return;

    if (!m_subtitle) {
        m_subtitle = m_scene->addText("", QFont("黑体", 13));
        m_subtitle->setDefaultTextColor(m_palDim);
        m_subtitle->setZValue(1000);
        m_subtitle->setOpacity(0.0);
        m_subtitle->setPos(260, 540);
    }

    Anim::fade(this, m_subtitle, 0.0, 120, [=]() {
        if (!m_subtitle) return;
        m_subtitle->setPlainText(subs[sel]);
        m_subtitle->setDefaultTextColor(m_palDim);
        Anim::fade(this, m_subtitle, 0.7, 250);
    });

    if (!m_subtitleTimer) {
        m_subtitleTimer = new QTimer(this);
        int *tick = new int(0);
        connect(m_subtitleTimer, &QTimer::timeout, this, [=]() mutable {
            if (m_subtitle && m_state != State::Inactive) {
                (*tick)++;
                qreal alpha = 0.5 + 0.2 * std::sin((*tick) * 0.08);
                m_subtitle->setOpacity(alpha);
            }
        });
        m_subtitleTimer->start(40);
    }
}

// 清理
void PauseMenu::clearAll() {
    if (m_overlay) { m_scene->removeItem(m_overlay); delete m_overlay; m_overlay = nullptr; }
    if (m_title) { m_scene->removeItem(m_title); delete m_title; m_title = nullptr; }
    if (m_confirmText) { m_scene->removeItem(m_confirmText); delete m_confirmText; m_confirmText = nullptr; }
    if (m_hintText) { m_scene->removeItem(m_hintText); delete m_hintText; m_hintText = nullptr; }
    if (m_subtitle) { m_scene->removeItem(m_subtitle); delete m_subtitle; m_subtitle = nullptr; }
    if (m_subtitleTimer) { m_subtitleTimer->stop(); delete m_subtitleTimer; m_subtitleTimer = nullptr; }
    // 光标保留（不随 clearAll 删除），只隐藏
    if (m_cursor) {
        if (m_cursor->scene()) m_scene->removeItem(m_cursor);
        m_cursor->setVisible(true);
    }
    for (auto *item : m_items) { m_scene->removeItem(item); delete item; }
    m_items.clear();
    for (auto *panel : m_panels) {
        if (panel->scene()) m_scene->removeItem(panel);
        delete panel;
    }
    m_panels.clear();
    m_optionCount = 0;
}
