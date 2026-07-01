#include "interactionsystem.h"
#include "dialoguesystem.h"
#include "grid.h"

#include <QFont>
#include <QTimer>
#include <algorithm>

InteractionSystem::InteractionSystem(QObject *parent)
    : QObject(parent)
{
}

void InteractionSystem::registerInteraction(const Interaction &inter) {
    m_interactions.append(inter);
}

Interaction *InteractionSystem::findInteraction(QPoint gridPos) {
    for (auto &inter : m_interactions)
        if (inter.gridPos == gridPos) return &inter;
    return nullptr;
}

bool InteractionSystem::tryInteract(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                                     QList<QGraphicsTextItem*> &pushables,
                                     QList<QGraphicsTextItem*> &waters,
                                     bool &inputEnabled) {
    Q_UNUSED(inputEnabled);
    if (m_active || !player || !arrow || !m_scene) return false;

    QPoint playerGrid = Grid::toGrid(player->pos());
    // 箭头方向
    QString a = arrow->toPlainText();
    int adx = 1, ady = 0;
    if (a == QStringLiteral("◀"))      { adx = -1; ady = 0; }
    else if (a == QStringLiteral("▲")) { adx = 0;  ady = -1; }
    else if (a == QStringLiteral("▼")) { adx = 0;  ady = 1; }

    QPoint frontGrid = playerGrid + QPoint(adx, ady);

    Interaction *inter = findInteraction(frontGrid);
    if (!inter) return false;

    m_active = true;
    m_currentGrid = frontGrid;
    m_currentLineIdx = 0;
    m_currentFullText.clear();
    // 交互对话不锁移动

    emit interactionStarted(frontGrid);

    if (!inter->lines.isEmpty()) {
        showLine(inter->lines[0], 1, 13, pushables, waters);
    } else {
        m_active = false;
        emit interactionFinished(frontGrid);
    }

    return true;
}

void InteractionSystem::showLine(const InteractionLine &line, int startX, int gy,
                                  QList<QGraphicsTextItem*> &pushables,
                                  QList<QGraphicsTextItem*> &waters) {
    if (!m_scene) return;

    // 停掉上一行的揭示定时器
    if (m_revealTimer) { m_revealTimer->stop(); delete m_revealTimer; m_revealTimer = nullptr; }

    clearCurrentLine(pushables, waters);
    m_currentFullText = line.text;

    // 长代码用小字号
    QFont f(line.smallFont ? QStringLiteral("黑体") : QStringLiteral("黑体"),
            line.smallFont ? 11 : 22);
    qreal spacing = line.smallFont ? 14.0 : Grid::kSize;

    for (int i = 0; i < line.text.size(); ++i) {
        auto *ch = m_scene->addText(QString(line.text[i]), f);
        ch->setDefaultTextColor(Qt::white);
        ch->setPos(startX * Grid::kSize + i * spacing, gy * Grid::kSize);
        ch->setZValue(80);
        ch->setVisible(false);  // 先隐藏，逐字揭示
        m_currentChars.append(ch);

        if (line.pushableIndices.contains(i))
            pushables.append(ch);
        else
            waters.append(ch);
    }

    // 句末三角
    if (!m_cursorItem) {
        m_cursorItem = m_scene->addText(QStringLiteral("▼"), QFont(QStringLiteral("黑体"), 10));
        m_cursorItem->setDefaultTextColor(Qt::white);
        m_cursorItem->setZValue(81);
    }
    m_cursorItem->setVisible(false);

    // 启动逐字揭示
    m_revealIndex = 0;
    m_revealTimer = new QTimer(this);
    connect(m_revealTimer, &QTimer::timeout, this, &InteractionSystem::revealStep);
    m_revealTimer->start(60);
}

void InteractionSystem::revealStep() {
    if (m_revealIndex < m_currentChars.size()) {
        if (m_currentChars[m_revealIndex])
            m_currentChars[m_revealIndex]->setVisible(true);
        m_revealIndex++;
    } else {
        if (m_revealTimer) { m_revealTimer->stop(); delete m_revealTimer; m_revealTimer = nullptr; }
        // 显示三角
        if (m_cursorItem && !m_currentChars.isEmpty()) {
            auto *last = m_currentChars.last();
            if (last) {
                m_cursorItem->setPos(last->pos().x() + last->boundingRect().width() + 4,
                                     last->pos().y() + last->boundingRect().height() - 6);
                m_cursorItem->setVisible(true);
            }
        }
    }
}

void InteractionSystem::skipReveal() {
    if (!m_revealTimer) return;
    for (int i = m_revealIndex; i < m_currentChars.size(); ++i) {
        if (m_currentChars[i])
            m_currentChars[i]->setVisible(true);
    }
    m_revealIndex = m_currentChars.size();
    revealStep();  // 触发三角显示
}

void InteractionSystem::advance(QList<QGraphicsTextItem*> &pushables,
                                 QList<QGraphicsTextItem*> &waters,
                                 bool &inputEnabled) {
    Q_UNUSED(inputEnabled);
    if (!m_active) return;

    // 如果还在逐字揭示中 、 跳过揭示
    if (m_revealTimer) {
        skipReveal();
        return;
    }

    Interaction *inter = findInteraction(m_currentGrid);
    if (!inter) {
        cleanup(pushables, waters);
        return;
    }

    m_currentLineIdx++;

    if (m_currentLineIdx >= inter->lines.size()) {
        cleanup(pushables, waters);
        emit interactionFinished(m_currentGrid);
        return;
    }

    showLine(inter->lines[m_currentLineIdx], 1, 13, pushables, waters);
}

void InteractionSystem::notifyItemDeleted(QGraphicsTextItem *item) {
    m_currentChars.removeAll(item);
}

QString InteractionSystem::currentVisibleText() const {
    // 按 x 坐标排序后拼接，保证顺序正确
    QList<QPair<qreal, QString>> chars;
    for (auto *ch : m_currentChars) {
        if (ch && ch->isVisible())
            chars.append({ch->pos().x(), ch->toPlainText()});
    }
    std::sort(chars.begin(), chars.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    QString result;
    for (const auto &c : chars) result += c.second;
    return result;
}

void InteractionSystem::clearCurrentLine(QList<QGraphicsTextItem*> &pushables,
                                          QList<QGraphicsTextItem*> &waters) {
    if (m_revealTimer) { m_revealTimer->stop(); delete m_revealTimer; m_revealTimer = nullptr; }
    for (auto *ch : m_currentChars) {
        if (ch) {
            pushables.removeAll(ch);
            waters.removeAll(ch);
            if (ch->scene()) ch->scene()->removeItem(ch);
            delete ch;
        }
    }
    m_currentChars.clear();
    if (m_cursorItem) m_cursorItem->setVisible(false);
}

void InteractionSystem::cleanup(QList<QGraphicsTextItem*> &pushables,
                                 QList<QGraphicsTextItem*> &waters) {
    clearCurrentLine(pushables, waters);
    if (m_cursorItem) {
        if (m_cursorItem->scene()) m_cursorItem->scene()->removeItem(m_cursorItem);
        delete m_cursorItem;
        m_cursorItem = nullptr;
    }
    m_active = false;
    m_currentGrid = QPoint();
    m_currentLineIdx = 0;
    m_currentFullText.clear();
}
