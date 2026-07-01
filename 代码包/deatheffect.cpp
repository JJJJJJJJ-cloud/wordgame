#include "deatheffect.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "grid.h"

#include <QFont>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QTimer>

DeathEffect::DeathEffect(QObject *parent)
    : QObject(parent)
{
}

//  启动
void DeathEffect::play(QGraphicsTextItem *player,
                       const QStringList &deathDialogues,
                       const QStringList &epilogueLines,
                       std::function<void()> onDone,
                       const QColor &epilogueColor)
{
    if (!m_scene || !player || m_playing) return;
    m_playing = true;
    m_player = player;
    m_deathDialogues = deathDialogues;
    m_epilogueLines = epilogueLines;
    m_epilogueColor = epilogueColor;
    m_onDone = std::move(onDone);

    //底层半透明黑色遮罩
    m_overlay = m_scene->addRect(m_scene->sceneRect(),
                                  QPen(Qt::NoPen),
                                  QBrush(QColor(0, 0, 0, 180)));
    m_overlay->setZValue(150);
    m_overlay->setOpacity(0.0);

    Anim::variantAnim(this, 0.0, 1.0, 600, QEasingCurve::InCubic,
        [this](qreal v) { if (m_overlay) m_overlay->setOpacity(v); });

    animateFlash(0);
}

//  闪烁
void DeathEffect::animateFlash(int count) {
    if (!m_player || count >= 8) {
        animateFall();
        return;
    }

    qreal target = (count % 2 == 0) ? 0.2 : 1.0;
    Anim::fade(this, m_player, target, 100, [this, count]() {
        animateFlash(count + 1);
    });
}

//  旋转倒下
void DeathEffect::animateFall() {
    if (!m_player) { showDeathChars(); return; }

    m_player->setTransformOriginPoint(m_player->boundingRect().center());
    auto *rot = Anim::prop(this, m_player, "rotation", 500, 90.0, QEasingCurve::InOutCubic);
    connect(rot, &QPropertyAnimation::finished, this, [this]() { showDeathChars(); });
    rot->start(QAbstractAnimation::DeleteWhenStopped);
}

//  "赤"字浮现（顶层 z=200，高于遮罩）
void DeathEffect::showDeathChars() {
    if (!m_player || !m_scene) { playDeathDialogue(0); return; }

    QPointF pp = m_player->pos();
    QFont chiFont(QStringLiteral("黑体"), 18);
    QColor darkRed(180, 30, 30);

    QList<QPointF> offsets = {
        {Grid::kSize,       -Grid::kSize * 0.5f},
        {-Grid::kSize * 0.5f, Grid::kSize * 0.5f},
        {Grid::kSize * 1.5f,  Grid::kSize * 0.5f},
        {Grid::kSize * 0.5f,  Grid::kSize * 1.2f}
    };

    for (const auto &off : offsets) {
        auto *chi = m_scene->addText(QStringLiteral("赤"), chiFont);
        chi->setDefaultTextColor(darkRed);
        chi->setPos(pp + off);
        chi->setOpacity(0.0);
        chi->setZValue(200);

        Anim::fade(this, chi, 0.85, 800);

        m_tempItems.append(chi);
    }

    QTimer::singleShot(600, this, [this]() { playDeathDialogue(0); });
}

//  死亡对话（逐句打字机播放，z=200 在遮罩之上）
void DeathEffect::playDeathDialogue(int idx) {
    if (idx >= m_deathDialogues.size()) {
        QTimer::singleShot(400, this, [this]() { showEpilogue(0); });
        return;
    }

    if (!m_dialogueSystem) {
        playDeathDialogue(idx + 1);
        return;
    }

    // 先用小延迟关掉上一句残留的对话，避免重叠
    m_dialogueSystem->cleanup();

    m_dialogueSystem->display(m_deathDialogues[idx], 2, 12, 18, [this, idx]() {
        playDeathDialogue(idx + 1);
    });

    // 把对话文字和光标提升到遮罩之上（死亡对话保持白色）
    m_dialogueSystem->setDisplayZValue(200);
    m_dialogueSystem->setDisplayColor(Qt::white);
}

//  结尾语（序列播放，z=200）
void DeathEffect::showEpilogue(int idx) {
    if (idx >= m_epilogueLines.size()) {
        finish();
        return;
    }

    if (!m_dialogueSystem) {
        showEpilogue(idx + 1);
        return;
    }

    m_dialogueSystem->cleanup();

    m_dialogueSystem->display(m_epilogueLines[idx], 1, 11, 16, [this, idx]() {
        showEpilogue(idx + 1);
    });

    m_dialogueSystem->setDisplayZValue(200);
    m_dialogueSystem->setDisplayColor(m_epilogueColor);
}

//  结束
void DeathEffect::finish() {
    // 不移除遮罩和文字，让玩家在暂停前能阅读最后一句
    m_playing = false;
    if (m_onDone) {
        auto cb = m_onDone;
        m_onDone = nullptr;
        cb();
    }
}

//  清理
void DeathEffect::cleanup() {
    if (m_dialogueSystem) {
        m_dialogueSystem->cleanup();
        m_dialogueSystem->setDisplayColor(Qt::white);
    }

    for (auto *item : m_tempItems) {
        if (item && item->scene()) item->scene()->removeItem(item);
        delete item;
    }
    m_tempItems.clear();

    if (m_overlay) {
        if (m_overlay->scene()) m_overlay->scene()->removeItem(m_overlay);
        delete m_overlay;
        m_overlay = nullptr;
    }

    m_player = nullptr;
    m_deathDialogues.clear();
    m_epilogueLines.clear();
    m_onDone = nullptr;
    m_playing = false;
}
