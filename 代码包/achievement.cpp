#include "achievement.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

AchievementManager *AchievementManager::s_instance = nullptr;

//成就定义表
const QList<AchievementInfo> AchievementManager::kAchievements = {
    {AchievementId::KaDian,
     QStringLiteral("卡点"),
     QStringLiteral("事实上，九点来到学一依旧可以买到鸡汤面，但这是作者的游戏，作者不允许。"),
     2, false},
    {AchievementId::KunShiTang,
     QStringLiteral("被困食堂之人"),
     QStringLiteral("你见过上午十点的学一吗？"),
     2, false},
    {AchievementId::KaGuan,
     QStringLiteral("卡关"),
     QStringLiteral("作者邀请过很多人测试游戏，每一位玩家不约而同地在这里卡关。"),
     3, false},
    {AchievementId::BaoLiuJieMu,
     QStringLiteral("保留节目"),
     QStringLiteral("被电源线绊倒，不得不品的一环。"),
     4, false},
    {AchievementId::HaoQiXin,
     QStringLiteral("好奇心"),
     QStringLiteral("泪目！你真的尝试了钓上所有物品！"),
     5, false},
    {AchievementId::DaDaDa,
     QStringLiteral("诶，云朵！"),
     QStringLiteral("哒哒哒哒哒..."),
     6, false},
    {AchievementId::ReBuQi,
     QStringLiteral("惹不起，总能躲得起吧？"),
     QStringLiteral("宝子这一关不弹反也可以通关..."),
     7, false},
    {AchievementId::BieXiangTaiDuo,
     QStringLiteral("烫烫烫。"),
     QStringLiteral("别想太多，向前冲吧!"),
     8, false},
    {AchievementId::ChuJianSha,
     QStringLiteral("初见杀"),
     QStringLiteral("所有内测玩家都被初见秒了。"),
     9, false},
    {AchievementId::JieShuLe,
     QStringLiteral("结束了？"),
     QStringLiteral("击败朗·昂索。"),
     10, false},
    {AchievementId::GuiMeng,
     QStringLiteral("归梦"),
     QStringLiteral("给没有看懂的uu门解释一下，\"林\"和\"夕\"组成了\"梦\"哦。"),
     11, false},
    {AchievementId::YiZhouMu,
     QStringLiteral("一周目"),
     QStringLiteral("二周目怎么设计呢？（作者沉思中...）"),
     12, false},
};

//单例
AchievementManager *AchievementManager::instance() {
    if (!s_instance) {
        s_instance = new AchievementManager();
        s_instance->init();
    }
    return s_instance;
}

AchievementManager::AchievementManager(QObject *parent)
    : QObject(parent)
{
}

//初始化/加载
void AchievementManager::init() {
    // 全部初始化为未解锁
    for (const auto &info : kAchievements)
        m_unlocked[info.id] = false;
    load();
}

static QString achievementsPath() {
    QString dir = QCoreApplication::applicationDirPath() + "/saves";
    QDir().mkpath(dir);
    return dir + "/achievements.json";
}

void AchievementManager::load() {
    QFile f(achievementsPath());
    if (!f.open(QIODevice::ReadOnly))
        return;

    QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    QJsonObject ach = root["achievements"].toObject();

    // 仅当 JSON 中明确为 true 时才覆盖默认的 false
    for (auto it = ach.begin(); it != ach.end(); ++it) {
        // 按字符串 key 匹配到枚举
        const QString key = it.key();
        bool val = it.value().toBool();
        if (!val) continue;

        for (const auto &info : kAchievements) {
            // 简单匹配：用枚举名转字符串比较
            if (key == QString::number(static_cast<int>(info.id))) {
                m_unlocked[info.id] = true;
                break;
            }
        }
    }
    f.close();
}

void AchievementManager::save() {
    QJsonObject ach;
    for (auto it = m_unlocked.begin(); it != m_unlocked.end(); ++it) {
        ach[QString::number(static_cast<int>(it.key()))] = it.value();
    }
    QJsonObject root;
    root["achievements"] = ach;

    QFile f(achievementsPath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(root).toJson());
        f.close();
    }
}

//解锁
void AchievementManager::unlock(AchievementId id) {
    if (!m_unlocked.contains(id)) return;
    if (m_unlocked[id]) return;   // 已解锁，不重复触发

    m_unlocked[id] = true;
    save();

    // 找到对应的 info 结构体并发射信号
    for (const auto &info : kAchievements) {
        if (info.id == id) {
            AchievementInfo unlockedInfo = info;
            unlockedInfo.unlocked = true;
            emit achievementUnlocked(unlockedInfo);
            return;
        }
    }
}

//查询
bool AchievementManager::isUnlocked(AchievementId id) const {
    return m_unlocked.value(id, false);
}

QList<AchievementInfo> AchievementManager::allAchievements() const {
    QList<AchievementInfo> result;
    for (const auto &info : kAchievements) {
        AchievementInfo copy = info;
        copy.unlocked = m_unlocked.value(info.id, false);
        result.append(copy);
    }
    return result;
}
