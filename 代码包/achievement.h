#ifndef ACHIEVEMENT_H
#define ACHIEVEMENT_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>

enum class AchievementId {
    KaDian,           // 卡点 — Level 2 改为"早上九点"
    KunShiTang,       // 被困食堂之人 — Level 2 改为"早上十点"
    KaGuan,           // 卡关 — Level 3 "车"推到 y=9
    BaoLiuJieMu,      // 保留节目 — Level 4 被电源线绊倒
    HaoQiXin,         // 好奇心 — Level 5 钓起全部物品后对话
    DaDaDa,           // 诶，云朵！ — Level 6 碰到云朵
    ReBuQi,           // 惹不起，总能躲得起吧？ — Level 7 HP 归零一次
    BieXiangTaiDuo,   // 别想太多，向前冲吧！ — Level 8 F键与"纸"交互
    ChuJianSha,       // 初见杀 — Level 9 死亡即可达成
    JieShuLe,         // 结束了？ — 通关 Level 10
    GuiMeng,          // 归梦 — 通关 Level 11
    YiZhouMu          // 一周目 — 通关 Level 12
};

struct AchievementInfo {
    AchievementId id;
    QString name;          // 成就名
    QString description;   // 成就概况
    int level;             // 所在关卡编号
    bool unlocked;
};

class AchievementManager : public QObject
{
    Q_OBJECT
public:
    static AchievementManager* instance();

    void init();                              // 从 JSON 加载
    void unlock(AchievementId id);            // 解锁成就（仅首次有效）
    bool isUnlocked(AchievementId id) const;  // 查询是否已解锁
    QList<AchievementInfo> allAchievements() const;  // 获取全部成就（含解锁状态）

signals:
    void achievementUnlocked(const AchievementInfo &info);

private:
    explicit AchievementManager(QObject *parent = nullptr);

    void load();
    void save();

    QMap<AchievementId, bool> m_unlocked;
    static const QList<AchievementInfo> kAchievements;  // 成就定义表

    static AchievementManager *s_instance;
};

#endif // ACHIEVEMENT_H
