#include "levelbase.h"
#include "dialoguesystem.h"
#include "grid.h"
#include "animutils.h"

//  对话辅助
void LevelBase::playSequence(const QStringList &lines,
                             std::function<void()> onDone) {
    if (!m_dialogueSystem) {
        if (onDone) onDone();
        return;
    }
    m_dialogueSystem->showSequence(lines, std::move(onDone));
}

void LevelBase::playSingle(const QString &text, int gx, int gy, int maxChars,
                           std::function<void()> onDone) {
    if (!m_dialogueSystem) {
        if (onDone) onDone();
        return;
    }
    m_dialogueSystem->display(text, gx, gy, maxChars, std::move(onDone));
}

//  箭头方向解析（▶◀▲▼ 、 dx/dy）
QPoint LevelBase::arrowDir(const QString &arrowText) {
    if (arrowText == QStringLiteral("◀"))      return {-1, 0};
    if (arrowText == QStringLiteral("▲"))      return {0, -1};
    if (arrowText == QStringLiteral("▼"))      return {0, 1};
    return {1, 0};  // 默认 ▶
}

//  itemAtGrid
QGraphicsTextItem *LevelBase::itemAtGrid(QPoint gp, const QString &text) const {
    if (!m_scene) return nullptr;
    for (auto *item : m_scene->items()) {
        if (auto *ti = dynamic_cast<QGraphicsTextItem*>(item)) {
            if (Grid::toGrid(ti->pos()) == gp) {
                if (text.isEmpty() || ti->toPlainText() == text)
                    return ti;
            }
        }
    }
    return nullptr;
}

//  自动行走
void LevelBase::autoWalkTo(QPoint target, std::function<void()> onDone) {
    if (m_autoWalkTimer) {
        m_autoWalkTimer->stop();
        delete m_autoWalkTimer;
        m_autoWalkTimer = nullptr;
    }
    m_autoWalkTarget = target;
    m_autoWalkDone = std::move(onDone);

    m_autoWalkTimer = new QTimer(this);
    connect(m_autoWalkTimer, &QTimer::timeout, this, &LevelBase::autoWalkStep);
    m_autoWalkTimer->start(150);
}

void LevelBase::autoWalkStep() {
    // 子类应自行查找 player 并实现具体移动逻辑
    // 默认实现为空（Level12 有 walkTo() 实现，Level9 有自己的 autoWalkStep）
}

//  清理（子类应调用 LevelBase::cleanup() 并处理自有资源）
void LevelBase::cleanup() {
    if (m_autoWalkTimer) {
        m_autoWalkTimer->stop();
        delete m_autoWalkTimer;
        m_autoWalkTimer = nullptr;
    }
    m_autoWalkTarget = {-1, -1};
    m_autoWalkDone = nullptr;
}
