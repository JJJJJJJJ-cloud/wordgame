#ifndef SAVEMANAGER_H
#define SAVEMANAGER_H

#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

struct SaveData {
    int level = 0;
    int playerGx = 0, playerGy = 0;
    int dirX = 1, dirY = 0;
    struct Item { int x, y; QString text; };
    QList<Item> pushables;
    QStringList deletedSpecials;
    int currentDialogueIndex = 0;
    // 第二关特定状态
    QString phraseAtPos17;  // "早上七点"中的(17,1)位置文字

    QJsonObject toJson() const {
        QJsonObject o;
        o["level"] = level;
        o["playerGx"] = playerGx; o["playerGy"] = playerGy;
        o["dirX"] = dirX; o["dirY"] = dirY;
        QJsonArray pa;
        for (auto &p : pushables) {
            QJsonObject po;
            po["x"] = p.x; po["y"] = p.y; po["text"] = p.text;
            pa.append(po);
        }
        o["pushables"] = pa;
        QJsonArray ds;
        for (auto &s : deletedSpecials) ds.append(s);
        o["deletedSpecials"] = ds;
        o["currentDialogueIndex"] = currentDialogueIndex;
        o["phraseAtPos17"] = phraseAtPos17;
        return o;
    }
};

class SaveManager {
public:
    static QString savePath(const QString &slot) {
        QString dir = QCoreApplication::applicationDirPath() + "/saves";
        QDir().mkpath(dir);
        return dir + "/save_" + slot + ".json";
    }

    static void save(const QString &slot, const SaveData &data) {
        QFile f(savePath(slot));
        if (f.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(data.toJson());
            f.write(doc.toJson());
            f.close();
        }
    }

    static bool exists(const QString &slot) {
        return QFile::exists(savePath(slot));
    }

    // —— 进度追踪（到达过的最高关卡） ——
    static void saveProgress(int highestLevel) {
        QJsonObject o;
        o["highestLevel"] = highestLevel;
        QString dir = QCoreApplication::applicationDirPath() + "/saves";
        QDir().mkpath(dir);
        QFile f(dir + "/progress.json");
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QJsonDocument(o).toJson());
            f.close();
        }
    }

    static int loadProgress() {
        QString dir = QCoreApplication::applicationDirPath() + "/saves";
        QFile f(dir + "/progress.json");
        if (f.open(QIODevice::ReadOnly)) {
            int h = QJsonDocument::fromJson(f.readAll()).object()["highestLevel"].toInt();
            f.close();
            return (h > 0 && h <= 12) ? h : 1;
        }
        return 1;  // 默认：第 1 关始终可用
    }
};

#endif
