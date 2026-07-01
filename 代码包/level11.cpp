#include "level11.h"
#include "animutils.h"
#include "dialoguesystem.h"
#include "grid.h"

#include <QFont>
#include <QFontMetrics>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QVariantAnimation>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <cmath>
#include <cstdlib>

//  构造函数
Level11::Level11(QObject *parent)
    : LevelBase(parent)
{
}

//  入场序列："我"从左上角飘到 (10,11)
void Level11::startEntry(QGraphicsTextItem *player, QGraphicsTextItem *arrow,
                          bool &inputEnabled) {
    if (!m_scene || !player) return;
    m_phase = Phase::Entry;
    m_player = player;
    m_arrow = arrow;
    m_inputEnabledPtr = &inputEnabled;
    inputEnabled = false;
    if (arrow) arrow->setVisible(false);

    // 固定对话显示位置，防止继承上一关的 setSequencePos 状态
    m_dialogueSystem->setSequencePos(1, 12, 18);

    // 玩家从 (0,0) 渐显并飘移到 (10,11)
    player->setOpacity(0.0);
    player->setPos(Grid::toPixel(0, 0));

    QPointF target = Grid::toPixel(10, 11);
    Anim::parallel(this, {
        Anim::prop(this, player, "opacity", 600, 1.0),
        Anim::prop(this, player, "pos", 1800, target, QEasingCurve::InOutCubic)
    }, [this]() {
        collectDragonChars();
        collectTreeChars();
        startDialogue1();
    });
}

//  收集龙字符，按 y 分组
void Level11::collectDragonChars() {
    m_dragonByY.clear();
    m_dragonYValues.clear();

    // 龙的位置坐标（来自关卡设计）
    const QSet<QPair<int,int>> dragonPositions = {
        {5,7},{6,6},{7,5},{8,2},{8,5},{9,2},{9,3},{9,5},{9,6},
        {9,7},{10,3},{10,4},{10,5},{10,6},{10,7},{10,8},{10,9},
        {11,2},{11,3},{11,5},{11,6},{11,7},{12,2},{12,5},{13,5},
        {14,6},{15,7}
    };

    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (!ti) continue;
        if (ti->toPlainText() != QStringLiteral("龙")) continue;
        QPoint g = Grid::toGrid(ti->pos());
        if (dragonPositions.contains({g.x(), g.y()})) {
            m_dragonByY[g.y()].append(ti);
        }
    }

    m_dragonYValues = m_dragonByY.keys();
    std::sort(m_dragonYValues.begin(), m_dragonYValues.end());
}

//  收集"林"字符
void Level11::collectTreeChars() {
    m_trees.clear();
    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (!ti) continue;
        if (ti->toPlainText() == QStringLiteral("林")) {
            m_trees.append(ti);
        }
    }
}

//  第一阶段对话：交心
void Level11::startDialogue1() {
    m_phase = Phase::Dialogue1;

    QStringList lines = {
        QStringLiteral("朗·昂索：怎……么……可……能……"),
        QStringLiteral("吾所行之道，岂非大义？"),
        QStringLiteral("我：你错在把人们恐惧的来源搞反了。"),
        QStringLiteral("你以为，WA 来自电脑。关了电脑，就没有 WA。"),
        QStringLiteral("问题从来都不源自机房。问题是人如何看待WA。"),
        QStringLiteral("但你的存在恰恰证明了，恐惧本身，值得被看见。"),
        QStringLiteral("你不需要采取如此极端的手段，"),
        QStringLiteral("你只需要让人们知道，WA本身并不可耻。"),
        QStringLiteral("错误和不确定，本身就是道路的一部分。"),
        QStringLiteral("在终点回头看，那些弯路也曾是风景。"),
        QStringLiteral("朗·昂索：既如此..."),
        QStringLiteral("指引凡人摆脱 WA 之阴影，直面未来之重任..."),
        QStringLiteral("...便交予你了。"),
    };

    playSequence(lines, [this]() {
        m_dialogueSystem->cleanup();
        startDragonToAC();
    });
}

//  龙 、 AC：逐排蜕变（y=2 、 y=7，每排间隔 0.5s）
void Level11::startDragonToAC() {
    m_phase = Phase::DragonToAC;
    animateDragonRow(0);
}

void Level11::animateDragonRow(int rowIndex) {
    if (rowIndex >= m_dragonYValues.size()) {
        // 所有排处理完毕 、 进入日、夕
        QTimer::singleShot(300, this, [this]() { startSunToXi(); });
        return;
    }

    int y = m_dragonYValues[rowIndex];
    const auto &dragons = m_dragonByY[y];

    // ① 龙渐隐（0.5s）
    QList<QPropertyAnimation*> anims;
    for (auto *d : dragons) {
        if (!d || !d->scene()) continue;
        anims.append(Anim::prop(this, d, "opacity", 500, 0.0));
    }

    Anim::parallel(this, anims, [this, y, dragons, rowIndex]() {
        // ② 在原位置生成蓝色"AC"
        QFont acFont(QStringLiteral("黑体"), 20, QFont::Bold);
        QColor acColor(80, 180, 255);
        QList<QGraphicsTextItem*> acItems;

        for (auto *d : dragons) {
            if (!d) continue;
            QPointF pos = d->pos();
            if (d->scene()) d->scene()->removeItem(d);
            delete d;

            auto *ac = m_scene->addText(QStringLiteral("AC"), acFont);
            ac->setDefaultTextColor(acColor);
            ac->setPos(pos);
            ac->setOpacity(0.0);
            ac->setZValue(20);
            acItems.append(ac);
            m_tempItems.append(ac);
            Anim::fade(this, ac, 1.0, 300);
        }

        // ③ 0.5s 后 AC 渐隐
        QTimer::singleShot(500, this, [this, acItems]() {
            for (auto *ac : acItems) {
                if (!ac || !ac->scene()) continue;
                Anim::fade(this, ac, 0.0, 400, [this, ac]() {
                    if (ac && ac->scene()) {
                        m_scene->removeItem(ac);
                        m_tempItems.removeAll(ac);
                        delete ac;
                    }
                }, QEasingCurve::InCubic);
            }
        });
    });

    // 下一排：0.5s 后
    QTimer::singleShot(500, this, [this, rowIndex]() {
        animateDragonRow(rowIndex + 1);
    });
}

//  日、夕："日"从 (2,2) 下移至 (2,3)，(2,2) 渐显"夕"
void Level11::startSunToXi() {
    m_phase = Phase::SunToXi;

    // 查找"日"，并染上与"夕"相同的暖橙色
    for (auto *item : m_scene->items()) {
        auto *ti = dynamic_cast<QGraphicsTextItem*>(item);
        if (!ti) continue;
        if (ti->toPlainText() == QStringLiteral("日")
            && Grid::toGrid(ti->pos()) == QPoint(2, 2)) {
            m_sun = ti;
            m_sun->setDefaultTextColor(QColor(255, 180, 60));  // 与"夕"一致的暖橙
            break;
        }
    }

    if (!m_sun) {
        startDialogue2();
        return;
    }

    // "日"缓缓下移一格
    Anim::slide(this, m_sun, Grid::toPixel(2, 3), 1200, [this]() {
        // 在 (2,2) 创建"夕"，渐显
        QFont xiFont(QStringLiteral("黑体"), 24, QFont::Bold);
        m_xi = m_scene->addText(QStringLiteral("夕"), xiFont);
        m_xi->setDefaultTextColor(QColor(255, 180, 60));
        m_xi->setPos(Grid::toPixel(2, 2));
        m_xi->setOpacity(0.0);
        m_xi->setZValue(15);
        m_tempItems.append(m_xi);

        Anim::fade(this, m_xi, 1.0, 800, [this]() {
            QTimer::singleShot(400, this, [this]() { startDialogue2(); });
        });
    }, QEasingCurve::InOutCubic);
}

//  第二阶段对话：夕阳叙事
void Level11::startDialogue2() {
    m_phase = Phase::Dialogue2;

    QStringList lines = {
        QStringLiteral("夕阳西下，这不平凡的一天终于要落下帷幕了..."),
        QStringLiteral("太以编辑？恶龙？神器？身怀绝技的老师？"),
        QStringLiteral("这一切太过离奇，宛如梦幻。"),
        QStringLiteral("梦..."),
        QStringLiteral("我望向缓缓下沉的夕阳，它温和地散发余晖，"),
        QStringLiteral("将湖畔的森林染上属于薄暮的色彩。"),
    };

    playSequence(lines, [this]() {
        m_dialogueSystem->cleanup();
        startXiGlow();
    });
}

//  夕辉光芒：8 条橙色射线从"夕"中心发散
void Level11::startXiGlow() {
    m_phase = Phase::XiGlow;

    if (!m_xi || !m_xi->scene()) {
        // "夕"已被清理，跳过
        QTimer::singleShot(500, this, [this]() { startTreesToDream(); });
        return;
    }

    // 给"夕"添加发光效果
    auto *glow = new QGraphicsDropShadowEffect;
    glow->setBlurRadius(25);
    glow->setColor(QColor(255, 150, 30, 220));
    glow->setOffset(0, 0);
    m_xi->setGraphicsEffect(glow);

    // 计算"夕"的中心位置
    QFontMetrics fm(QFont(QStringLiteral("黑体"), 24));
    qreal cw = fm.horizontalAdvance(QStringLiteral("夕"));
    qreal ch = fm.height();
    QPointF center = m_xi->pos() + QPointF(cw / 2.0, ch / 2.0);

    // 8 条射线，角度均匀分布（加上小随机偏移）
    const int numRays = 8;
    const qreal rayLength = 130.0;
    QPen rayPen(QColor(255, 160, 40, 200), 1.8);

    for (int i = 0; i < numRays; ++i) {
        qreal angle = (i * 360.0 / numRays + (std::rand() % 10 - 5)) * M_PI / 180.0;
        qreal dx = std::cos(angle) * rayLength;
        qreal dy = std::sin(angle) * rayLength;

        auto *ray = m_scene->addLine(
            center.x(), center.y(),
            center.x() + dx, center.y() + dy,
            rayPen);
        ray->setOpacity(0.0);
        ray->setZValue(14);
        m_rays.append(ray);

        // 渐显
        Anim::variantAnim(this, 0.0, 0.7, 600 + (std::rand() % 400),
            QEasingCurve::OutCubic,
            [ray](qreal v) { if (ray && ray->scene()) ray->setOpacity(v); });
    }

    // 3 秒后渐隐射线
    QTimer::singleShot(3000, this, [this]() { fadeOutRays(); });
}

void Level11::fadeOutRays() {
    for (auto *ray : m_rays) {
        if (!ray || !ray->scene()) continue;
        Anim::variantAnim(this, ray->opacity(), 0.0, 800, QEasingCurve::InCubic,
            [ray](qreal v) { if (ray && ray->scene()) ray->setOpacity(v); },
            [this, ray]() {
                if (ray && ray->scene()) {
                    m_scene->removeItem(ray);
                    m_rays.removeAll(ray);
                    delete ray;
                }
            });
    }

    // 光芒渐隐后启动林、梦
    QTimer::singleShot(1000, this, [this]() { startTreesToDream(); });
}

//  林+夕、梦：全场波浪式融合（4波，每波间隔 0.35s）
void Level11::startTreesToDream() {
    m_phase = Phase::TreesToDream;

    if (m_trees.isEmpty()) {
        QTimer::singleShot(1500, this, [this]() { finalFadeOut(); });
        return;
    }

    // 将树按 x 坐标排序
    std::sort(m_trees.begin(), m_trees.end(),
              [](QGraphicsTextItem *a, QGraphicsTextItem *b) {
        return a->pos().x() < b->pos().x();
    });

    // 4 波：x ∈ [0,4], [5,9], [10,14], [15,19]
    animateTreeWave(0);
}

void Level11::animateTreeWave(int waveIndex) {
    const int kWaveCount = 4;
    const int kColsPerWave = 5;  // 20 cols / 4 waves

    if (waveIndex >= kWaveCount) {
        // 所有波次完成 、 最终黑屏
        QTimer::singleShot(600, this, [this]() { finalFadeOut(); });
        return;
    }

    int xMin = waveIndex * kColsPerWave;
    int xMax = xMin + kColsPerWave - 1;

    // 收集当前波次的树，同时保存它们的网格位置（在删除前）
    struct TreeInfo {
        QGraphicsTextItem *item;
        QPoint gridPos;  // 保存网格坐标，供后续创建"梦"使用
    };
    QList<TreeInfo> waveTreeInfos;

    for (auto *tree : m_trees) {
        if (!tree || !tree->scene()) continue;
        int tx = Grid::toGrid(tree->pos()).x();
        if (tx >= xMin && tx <= xMax)
            waveTreeInfos.append({tree, Grid::toGrid(tree->pos())});
    }

    if (waveTreeInfos.isEmpty()) {
        animateTreeWave(waveIndex + 1);
        return;
    }

    // 每个"林"下方渐显"夕"（0.4s）
    QList<QGraphicsTextItem*> waveXiItems;
    QFont xiFont(QStringLiteral("黑体"), 22, QFont::Bold);
    QColor xiColor(255, 180, 60);

    for (const auto &info : waveTreeInfos) {
        auto *xi = m_scene->addText(QStringLiteral("夕"), xiFont);
        xi->setDefaultTextColor(xiColor);
        xi->setPos(Grid::toPixel(info.gridPos.x(), info.gridPos.y() + 1));
        xi->setOpacity(0.0);
        xi->setZValue(10);
        waveXiItems.append(xi);
        m_tempItems.append(xi);
        Anim::fade(this, xi, 1.0, 400);
    }

    // 0.7s 后，"林"和"夕"同时渐隐，"梦"渐显
    QTimer::singleShot(700, this, [this, waveTreeInfos, waveXiItems, waveIndex]() {
        QList<QPropertyAnimation*> fadeAnims;
        for (const auto &info : waveTreeInfos) {
            if (!info.item || !info.item->scene()) continue;
            fadeAnims.append(Anim::prop(this, info.item, "opacity", 350, 0.0, QEasingCurve::InCubic));
        }
        for (auto *xi : waveXiItems) {
            if (!xi || !xi->scene()) continue;
            fadeAnims.append(Anim::prop(this, xi, "opacity", 350, 0.0, QEasingCurve::InCubic));
        }

        Anim::parallel(this, fadeAnims, [this, waveTreeInfos, waveXiItems]() {
            for (const auto &info : waveTreeInfos) {
                if (!info.item) continue;
                if (info.item->scene()) m_scene->removeItem(info.item);
                if (m_watersPtr) m_watersPtr->removeAll(info.item);
                m_trees.removeAll(info.item);
                delete info.item;
            }
            for (auto *xi : waveXiItems) {
                if (!xi) continue;
                if (xi->scene()) m_scene->removeItem(xi);
                m_tempItems.removeAll(xi);
                delete xi;
            }

            QFont dreamFont(QStringLiteral("黑体"), 24, QFont::Bold);
            QColor dreamColor(255, 180, 60);

            for (const auto &info : waveTreeInfos) {
                auto *dream = m_scene->addText(QStringLiteral("梦"), dreamFont);
                dream->setDefaultTextColor(dreamColor);
                dream->setPos(Grid::toPixel(info.gridPos.x(), info.gridPos.y()));
                dream->setOpacity(0.0);
                dream->setZValue(8);
                m_tempItems.append(dream);
                Anim::fade(this, dream, 1.0, 400);
            }
        });
    });

    // 下一波：0.35s 后启动
    QTimer::singleShot(350, this, [this, waveIndex]() {
        animateTreeWave(waveIndex + 1);
    });
}

//  最终黑屏：全屏渐黑 、 关卡结束
void Level11::finalFadeOut() {
    m_phase = Phase::FadeOut;

    if (!m_scene) {
        emit level11Complete();
        return;
    }

    QGraphicsRectItem *fadeRect = m_scene->addRect(m_scene->sceneRect(),
                                                    QPen(Qt::NoPen),
                                                    QBrush(Qt::black));
    fadeRect->setOpacity(0.0);
    fadeRect->setZValue(999);

    Anim::variantAnim(this, 0.0, 1.0, 1000, QEasingCurve::InQuad,
        [fadeRect](qreal v) { fadeRect->setOpacity(v); },
        [this, fadeRect]() {
            m_scene->removeItem(fadeRect);
            delete fadeRect;
            m_phase = Phase::Done;
            emit level11Complete();
        });
}

//  临时文字辅助
QGraphicsTextItem *Level11::addTempText(const QString &text, int gx, int gy,
                                         int fontSize, QColor color,
                                         qreal opacity, qreal z) {
    if (!m_scene) return nullptr;
    QFont f(QStringLiteral("黑体"), fontSize);
    auto *item = m_scene->addText(text, f);
    item->setDefaultTextColor(color);
    item->setPos(Grid::toPixel(gx, gy));
    item->setOpacity(opacity);
    item->setZValue(z);
    m_tempItems.append(item);
    return item;
}

//  清理
void Level11::cleanup() {
    // 清理临时文字（AC / 梦 / 树下的夕 等）
    for (auto *item : m_tempItems) {
        if (item && item->scene()) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_tempItems.clear();

    // 清理光芒射线
    for (auto *ray : m_rays) {
        if (ray && ray->scene()) {
            m_scene->removeItem(ray);
            delete ray;
        }
    }
    m_rays.clear();

    // 清理数据结构
    m_dragonByY.clear();
    m_dragonYValues.clear();
    m_trees.clear();

    // 重置引用
    m_player = nullptr;
    m_arrow = nullptr;
    m_inputEnabledPtr = nullptr;
    m_watersPtr = nullptr;
    m_sun = nullptr;
    m_xi = nullptr;
    m_phase = Phase::Entry;

    LevelBase::cleanup();
}
