#include "leveldata.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// 内嵌 JSON 关卡数据
static const char kLevelsJson[] = R"JSON({
  "levels": [
    {
      "n": 1,
      "mechanic": "TextCombine",
      "items": [
        {"t": "我", "x": 2, "y": 10, "type": "player"}
      ],
      "specials": [
        {"t": "上锁的门", "x": 18, "y": 10}
      ],
      "combos": [
        {"push": "没", "target": "上锁的门", "result": "没上锁的门", "walkX": 17, "walkY": 10}
      ],
      "decor": [
        {"t": "床", "x": 2, "y": 11},
        {"t": "灯", "x": 17, "y": 2, "sway": true},
        {"t": "床", "x": 15, "y": 11},
        {"t": "桌", "x": 14, "y": 7},
        {"t": "窗", "x": 18, "y": 3},
        {"t": "书", "x": 16, "y": 6},
        {"t": "架", "x": 17, "y": 6},
        {"t": "床", "x": 1, "y": 4},
        {"t": "室友", "x": 1, "y": 3, "sway": true},
        {"t": "Z", "x": 2, "y": 2, "sway": true},
        {"t": "Z", "x": 3, "y": 1, "sway": true},
        {"t": "Z", "x": 4, "y": 2, "sway": true}
      ],
      "hint": "按E键推进对话，将「没」推动到「上锁的门」前吧！",
      "hintX": 0, "hintY": 14
    },
    {
      "n": 2,
      "mechanic": "PhraseReplace",
      "items": [
        {"t": "我", "x": 2, "y": 9, "type": "player"},
        {"t": "厨", "x": 18, "y": 9, "type": "scenery"}
      ],
      "waters": [
        {"t": "人", "x": 11, "y": 9},
        {"t": "人", "x": 12, "y": 9},
        {"t": "人", "x": 13, "y": 9},
        {"t": "人", "x": 14, "y": 9},
        {"t": "人", "x": 15, "y": 9},
        {"t": "人", "x": 16, "y": 9},
        {"t": "人", "x": 17, "y": 9}
      ],
      "phrases": [
        {"pos": [[3,12],[4,12],[5,12],[6,12]], "win": "早上八点", "bad": "早上九点"},
        {"pos": [[3,12],[4,12],[5,12],[6,12]], "win": "早上八点", "bad": "早上十点"}
      ],
      "fadeOut": [[11,9],[12,9],[13,9],[14,9],[15,9],[16,9],[17,9]],
      "winDialogue": "太好了，买到鸡汤面了！",
      "decor": [
        {"t": "食", "x": 0, "y": 0, "b": false},
        {"t": "堂", "x": 1, "y": 0, "b": false},
        {"t": "汤", "x": 16, "y": 8},
        {"t": "面", "x": 17, "y": 8},
        {"t": "~", "x": 16, "y": 7, "sway": true},
        {"t": "~", "x": 17, "y": 7, "sway": true},
        {"t": "~", "x": 16, "y": 6, "sway": true},
        {"t": "锅", "x": 18, "y": 7},
        {"t": "菜", "x": 17, "y": 6, "b": false},
        {"t": "菜", "x": 17, "y": 0, "b": false},
        {"t": "单", "x": 18, "y": 0, "b": false}
      ],
      "hint": "想想方法让人流消失，买到鸡汤面吧！",
      "hintX": 0, "hintY": 14
    },
    {
      "n": 3,
      "mechanic": "PhraseRearrange",
      "items": [
        {"t": "我", "x": 1, "y": 13, "type": "player"}
      ],
      "specials": [
        {"t": "车", "x": 6, "y": 1}
      ]
    },
    {
      "n": 4,
      "mechanic": "PureNarrative",
      "items": [
        {"t": "我", "x": 17, "y": 0, "type": "player"}
      ],
      "waters": [
        {"t": "黑", "x": 9, "y": 0},
        {"t": "板", "x": 10, "y": 0},
        {"t": "师", "x": 11, "y": 2},
        {"t": "门", "x": 17, "y": 0}
      ],
      "deskRows": [3, 5, 7],
      "dialogues": [
        "老师：同学们，你们知道吗...",
        "这个世界上存在极其罕见的能力：太以编辑。",
        "能力的持有者，其行为是直接以字幕显示的。",
        "因而他们能在概念意义上影响现实世界！",
        "（此时教室外传来疑似龙的吼声）",
        "老师：不好，是恶龙！",
        "你的内心：什么情况，恶龙？我在做梦么？",
        "老师：啊，这个字幕！",
        "我能看到你的字幕！你拥有太以编辑的能力！",
        "只有你才能击败恶龙，勇者！",
        "我：诶...我打恶龙，真的假的？！",
        "老师：但首先，你需要两件神器。跟我来！",
        "于是，我就这样不明所以地被老师拽走了。"
      ]
    },
    {
      "n": 5,
      "mechanic": "Fishing",
      "items": [
        {"t": "我", "x": -2, "y": 0, "type": "player"},
        {"t": "师", "x": -3, "y": 0, "type": "scenery"},
        {"t": "鸭子", "x": 6, "y": 10, "type": "scenery"},
        {"t": "手机", "x": 4, "y": 14, "type": "scenery"},
        {"t": "同学", "x": 8, "y": 14, "type": "scenery"},
        {"t": "共享单车", "x": 12, "y": 14, "type": "scenery"}
      ],
      "specials": [
        {"t": "贝克思贝斯之剑", "x": 16, "y": 14}
      ],
      "fishing": {
        "items": ["手机", "同学", "共享单车", "贝克思贝斯之剑"],
        "targetIndex": 3
      },
      "decor": [
        {"t": "未", "x": 1, "y": 1, "b": false},
        {"t": "名", "x": 2, "y": 1, "b": false},
        {"t": "湖", "x": 3, "y": 1, "b": false},
        {"t": "~", "x": 0, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 1, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 2, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 3, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 4, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 5, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 6, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 7, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 8, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 9, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 10, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 11, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 12, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 13, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 14, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 15, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 16, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 17, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 18, "y": 11, "b": false, "color": "#5078b4"},
        {"t": "~", "x": 19, "y": 11, "b": false, "color": "#5078b4"}
      ]
    },
    {
      "n": 6,
      "mechanic": "TowerClimb",
      "items": [
        {"t": "我", "x": 9, "y": 12, "type": "player"},
        {"t": "师", "x": 10, "y": 12, "type": "scenery"}
      ],
      "groundDots": true,
      "specials": [
        {"t": "恩特之盾", "x": 8, "y": 1}
      ],
      "platforms": [
        {"t": "所以请问怎么上去啊！", "x": 0, "y": 10},
        {"t": "咦，我怎么到半空中了？", "x": 9, "y": 8},
        {"t": "老师：这就是强大的太以编辑！", "x": 0, "y": 6},
        {"t": "我：哦哦哦哦哦哦哦！", "x": 10, "y": 3}
      ],
      "climbWin": [9, 1]
    },
    {
      "n": 7,
      "mechanic": "Standard",
      "win": "HasText",
      "winTarget": "__NO_WIN_TRIGGER__",
      "items": [
        {"t": "我", "x": -1, "y": 4, "type": "player"},
        {"t": "师", "x": -2, "y": 4, "type": "scenery"}
      ],
      "waters": [
        {"t": "没", "x": 7, "y": 6}, {"t": "开", "x": 7, "y": 7},
        {"t": "花", "x": 7, "y": 8}, {"t": "的", "x": 7, "y": 9},
        {"t": "树", "x": 7, "y": 10},
        {"t": "没", "x": 11, "y": 2}, {"t": "开", "x": 11, "y": 3},
        {"t": "花", "x": 11, "y": 4}, {"t": "的", "x": 11, "y": 5},
        {"t": "树", "x": 11, "y": 6},
        {"t": "没", "x": 16, "y": 4}, {"t": "开", "x": 16, "y": 5},
        {"t": "花", "x": 16, "y": 6}, {"t": "的", "x": 16, "y": 7},
        {"t": "树", "x": 16, "y": 8}
      ],
      "grasses": [
        [5,8],[9,7],[8,11],[9,4],[13,3],[12,6],[14,5],[18,6],[15,9]
      ],
      "banners": [
        {"t": "东", "x": 14, "y": 0},{"t": "侧", "x": 15, "y": 0},
        {"t": "门", "x": 16, "y": 0},{"t": "大", "x": 17, "y": 0},
        {"t": "草", "x": 18, "y": 0},{"t": "坪", "x": 19, "y": 0}
      ]
    },
    {
      "n": 8,
      "mechanic": "Standard",
      "win": "HasText",
      "winTarget": "__NO_WIN_TRIGGER__",
      "items": [
        {"t": "我", "x": -1, "y": 0, "type": "player"}
      ],
      "waters": [
        {"t": "桌", "x": 3, "y": 9},
        {"t": "纸", "x": 3, "y": 8},
        {"t": "椅", "x": 5, "y": 4, "rot": 180},
        {"t": "机", "x": 7, "y": 7, "rot": -90},
        {"t": "机", "x": 8, "y": 10},
        {"t": "门", "x": 10, "y": 0},
        {"t": "人", "x": 12, "y": 4},
        {"t": "门", "x": 15, "y": 14},
        {"t": "石", "x": 9, "y": 0}, {"t": "石", "x": 9, "y": 1},
        {"t": "石", "x": 10, "y": 1}, {"t": "石", "x": 11, "y": 1},
        {"t": "石", "x": 11, "y": 0},
        {"t": "人", "x": 15, "y": 11, "rot": 90},
        {"t": "人", "x": 16, "y": 6},
        {"t": "机", "x": 17, "y": 2, "rot": 90}
      ]
    },
    {
      "n": 9,
      "mechanic": "Standard",
      "win": "HasText",
      "winTarget": "__NO_WIN_TRIGGER__",
      "items": [
        {"t": "我", "x": -1, "y": -1, "type": "player"}
      ],
      "waters": [
        {"t": "门", "x": 10, "y": 14},
        {"t": "机", "x": 3, "y": 8, "rot": -90},
        {"t": "机", "x": 15, "y": 8, "rot": 90},
        {"t": "机", "x": 17, "y": 13},
        {"t": "机", "x": 2, "y": 13},
        {"t": "桌", "x": 1, "y": 10}, {"t": "桌", "x": 18, "y": 10},
        {"t": "椅", "x": 0, "y": 10, "rot": 180},
        {"t": "椅", "x": 19, "y": 10, "rot": 180}
      ],
      "dragonShape": [
        [5,5],[6,4],[7,3],[8,0],[8,3],[9,0],[9,1],[9,3],[9,4],
        [9,5],[10,1],[10,2],[10,3],[10,4],[10,5],[10,6],[10,7],
        [11,0],[11,1],[11,3],[11,4],[11,5],[12,0],[12,3],[13,3],
        [14,4],[15,5]
      ]
    },
    {
      "n": 10,
      "mechanic": "Standard",
      "win": "HasText",
      "winTarget": "__NO_WIN_TRIGGER__",
      "items": [
        {"t": "龙", "x": 0, "y": 3, "type": "scenery"},
        {"t": "我", "x": 16, "y": 3, "type": "player"}
      ]
    },
    {
      "n": 11,
      "mechanic": "Standard",
      "win": "HasText",
      "winTarget": "__NO_WIN_TRIGGER__",
      "items": [
        {"t": "我", "x": 0, "y": 0, "type": "player"},
        {"t": "日", "x": 2, "y": 2, "type": "scenery"}
      ],
      "dragonShape": [
        [5,7],[6,6],[7,5],[8,2],[8,5],[9,2],[9,3],[9,5],[9,6],
        [9,7],[10,3],[10,4],[10,5],[10,6],[10,7],[10,8],[10,9],
        [11,2],[11,3],[11,5],[11,6],[11,7],[12,2],[12,5],[13,5],
        [14,6],[15,7]
      ],
      "forestBorder": true
    },
    {
      "n": 12,
      "mechanic": "Standard",
      "win": "HasText",
      "winTarget": "__NO_WIN_TRIGGER__",
      "items": [
        {"t": "我", "x": 2, "y": 10, "type": "player"},
        {"t": "门", "x": 19, "y": 12, "type": "scenery"}
      ],
      "waters": [
        {"t": "床", "x": 2, "y": 11}, {"t": "灯", "x": 17, "y": 2},
        {"t": "床", "x": 15, "y": 11}, {"t": "桌", "x": 14, "y": 7},
        {"t": "窗", "x": 18, "y": 3}, {"t": "书", "x": 16, "y": 6},
        {"t": "架", "x": 17, "y": 6}, {"t": "床", "x": 1, "y": 4},
        {"t": "室友", "x": 1, "y": 3}, {"t": "Z", "x": 2, "y": 2},
        {"t": "Z", "x": 3, "y": 1}, {"t": "Z", "x": 4, "y": 2}
      ]
    }
  ]
})JSON";

// JSON 解析缓存
static QJsonArray s_levels;

static void ensureLoaded()
{
    if (!s_levels.isEmpty()) return;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(
        QByteArray(kLevelsJson, static_cast<int>(sizeof(kLevelsJson) - 1)), &err);
    if (err.error != QJsonParseError::NoError) {
        qCritical() << "[LevelData] JSON parse error:" << err.errorString()
                     << "at offset" << err.offset;
        return;
    }
    s_levels = doc.object().value("levels").toArray();
    qDebug() << "[LevelData] Loaded" << s_levels.size() << "levels from embedded JSON";
}

static QJsonObject levelJson(int n)
{
    ensureLoaded();
    for (int i = 0; i < s_levels.size(); ++i) {
        QJsonObject o = s_levels[i].toObject();
        if (o.value("n").toInt() == n) return o;
    }
    return {};
}

//枚举解析
static LevelData::Mechanic parseMechanic(const QString &s)
{
    if (s == "TextCombine")      return LevelData::TextCombine;
    if (s == "PhraseReplace")    return LevelData::PhraseReplace;
    if (s == "PhraseRearrange")  return LevelData::PhraseRearrange;
    if (s == "PureNarrative")    return LevelData::PureNarrative;
    if (s == "Fishing")          return LevelData::Fishing;
    if (s == "TowerClimb")       return LevelData::TowerClimb;
    return LevelData::Standard;
}

static LevelData::WinCondition parseWin(const QString &s)
{
    if (s == "NoText") return LevelData::NoText;
    return LevelData::HasText;
}

static LevelItem::Type parseType(const QString &s)
{
    if (s == "player")   return LevelItem::Player;
    if (s == "pushable") return LevelItem::Pushable;
    if (s == "water")    return LevelItem::Water;
    if (s == "scenery")  return LevelItem::Scenery;
    if (s == "special")  return LevelItem::Special;
    if (s == "trap")     return LevelItem::Trap;
    return LevelItem::Water; // default
}

//通用 JSON 、 LevelData 解析
static LevelData parseLevel(const QJsonObject &o)
{
    LevelData d;
    d.levelNumber = o.value("n").toInt();
    d.mechanic = parseMechanic(o.value("mechanic").toString());
    d.winCondition = parseWin(o.value("win").toString("HasText"));
    d.winTarget = o.value("winTarget").toString();

    // items 
    const QJsonArray items = o.value("items").toArray();
    for (int i = 0; i < items.size(); ++i) {
        QJsonObject it = items[i].toObject();
        LevelItem li;
        li.text = it.value("t").toString();
        li.gx   = it.value("x").toInt();
        li.gy   = it.value("y").toInt();
        li.type = parseType(it.value("type").toString("water"));
        li.rotation = it.value("rot").toInt();
        QString c = it.value("color").toString();
        if (!c.isEmpty()) li.customColor = QColor(c);
        d.items.append(li);
    }

    //specials (shortcut: auto-type=Special)
    const QJsonArray specials = o.value("specials").toArray();
    for (int i = 0; i < specials.size(); ++i) {
        QJsonObject it = specials[i].toObject();
        LevelItem li;
        li.text = it.value("t").toString();
        li.gx   = it.value("x").toInt();
        li.gy   = it.value("y").toInt();
        li.type = LevelItem::Special;
        d.items.append(li);
    }

    // decor (布景) 
    const QJsonArray decors = o.value("decor").toArray();
    for (int i = 0; i < decors.size(); ++i) {
        QJsonObject dc = decors[i].toObject();
        DecorItem di;
        di.text     = dc.value("t").toString();
        di.gx       = dc.value("x").toInt();
        di.gy       = dc.value("y").toInt();
        di.blocking = dc.value("b").toBool(true);   // 默认 true（碰撞体）
        di.swaying  = dc.value("sway").toBool(false);
        QString c = dc.value("color").toString();
        if (!c.isEmpty()) di.color = QColor(c);
        d.decors.append(di);
    }

    // ── waters (shortcut: auto-type=Water)
    const QJsonArray waters = o.value("waters").toArray();
    for (int i = 0; i < waters.size(); ++i) {
        QJsonObject it = waters[i].toObject();
        LevelItem li;
        li.text = it.value("t").toString();
        li.gx   = it.value("x").toInt();
        li.gy   = it.value("y").toInt();
        li.type = LevelItem::Water;
        li.rotation = it.value("rot").toInt();
        d.items.append(li);
    }

    //  level 4: desks 
    const QJsonArray deskRows = o.value("deskRows").toArray();
    for (int i = 0; i < deskRows.size(); ++i) {
        int y = deskRows[i].toInt();
        for (int x = 2; x <= 7; ++x)
            d.items.append({"桌", x, y, LevelItem::Water});
        for (int x = 9; x <= 15; ++x)
            d.items.append({"桌", x, y, LevelItem::Water});
    }

    //  level 6: ground dots 
    if (o.value("groundDots").toBool()) {
        for (int x = 0; x <= 19; ++x)
            d.items.append({"·", x, 13, LevelItem::Scenery});
    }

    // level 6: platforms (Special) 
    const QJsonArray platforms = o.value("platforms").toArray();
    for (int i = 0; i < platforms.size(); ++i) {
        QJsonObject p = platforms[i].toObject();
        d.items.append({p.value("t").toString(), p.value("x").toInt(),
                        p.value("y").toInt(), LevelItem::Special});
    }

    // level 7: grasses (浅绿色) 
    const QJsonArray grasses = o.value("grasses").toArray();
    for (int i = 0; i < grasses.size(); ++i) {
        QJsonArray pt = grasses[i].toArray();
        d.items.append({"艹", pt[0].toInt(), pt[1].toInt(),
                        LevelItem::Scenery, 0,
                        QColor(144, 238, 144)});
    }

    // level 7: banners 
    const QJsonArray banners = o.value("banners").toArray();
    for (int i = 0; i < banners.size(); ++i) {
        QJsonObject b = banners[i].toObject();
        d.items.append({b.value("t").toString(), b.value("x").toInt(),
                        b.value("y").toInt(), LevelItem::Scenery});
    }

    //dragon shape (L9 & L11) 
    const QJsonArray dragon = o.value("dragonShape").toArray();
    for (int i = 0; i < dragon.size(); ++i) {
        QJsonArray pt = dragon[i].toArray();
        d.items.append({"龙", pt[0].toInt(), pt[1].toInt(), LevelItem::Water});
    }

    // level 11: forest border (林) 
    if (o.value("forestBorder").toBool()) {
        for (int x = 0; x <= 19; ++x) {
            d.items.append({"林", x, 0, LevelItem::Water});
            d.items.append({"林", x, 13, LevelItem::Water});
        }
        const int edgeYs[] = {2, 4, 6, 8, 10};
        for (int y : edgeYs) {
            d.items.append({"林", 0, y, LevelItem::Water});
            d.items.append({"林", 19, y, LevelItem::Water});
        }
    }

    // combos 
    const QJsonArray combos = o.value("combos").toArray();
    for (int i = 0; i < combos.size(); ++i) {
        QJsonObject c = combos[i].toObject();
        ComboRule r;
        r.pushableText = c.value("push").toString();
        r.targetText   = c.value("target").toString();
        r.resultText   = c.value("result").toString();
        r.adjacencyDir = c.value("dir").toInt();
        r.autoWalkX    = c.value("walkX").toInt(-1);
        r.autoWalkY    = c.value("walkY").toInt(-1);
        d.comboRules.append(r);
    }

    // phrases 
    const QJsonArray phrases = o.value("phrases").toArray();
    for (int i = 0; i < phrases.size(); ++i) {
        QJsonObject p = phrases[i].toObject();
        PhraseCheckRule r;
        const QJsonArray pos = p.value("pos").toArray();
        for (int j = 0; j < pos.size(); ++j) {
            QJsonArray pt = pos[j].toArray();
            r.positions.append(QPoint(pt[0].toInt(), pt[1].toInt()));
        }
        r.winPhrase = p.value("win").toString();
        r.badPhrase = p.value("bad").toString();
        d.phraseRules.append(r);
    }

    // fadeOut
    const QJsonArray fadeOut = o.value("fadeOut").toArray();
    for (int i = 0; i < fadeOut.size(); ++i) {
        QJsonArray pt = fadeOut[i].toArray();
        d.fadeOutPositions.append(QPoint(pt[0].toInt(), pt[1].toInt()));
    }
    d.hasFadeOutSequence = !d.fadeOutPositions.isEmpty();

    // dialogues
    const QJsonArray dialogues = o.value("dialogues").toArray();
    for (int i = 0; i < dialogues.size(); ++i)
        d.dialogues.append(dialogues[i].toString());

    // simple scalar fields
    d.winDialogue = o.value("winDialogue").toString();
    d.tutorialHintText = o.value("hint").toString();
    d.hintGx = o.value("hintX").toInt(-1);
    d.hintGy = o.value("hintY").toInt(-1);

    //climb win
    QJsonArray cw = o.value("climbWin").toArray();
    if (cw.size() >= 2) {
        d.climbWinX = cw[0].toInt();
        d.climbWinY = cw[1].toInt();
    }

    // fishing
    QJsonObject fish = o.value("fishing").toObject();
    if (!fish.isEmpty()) {
        d.fishingRopeStartX = fish.value("ropeX").toInt(-1);
        const QJsonArray fitems = fish.value("items").toArray();
        for (int i = 0; i < fitems.size(); ++i)
            d.fishingItems.append(fitems[i].toString());
        d.fishingTargetIndex = fish.value("targetIndex").toInt(-1);
    }

    // traps
    const QJsonArray traps = o.value("traps").toArray();
    for (int i = 0; i < traps.size(); ++i) {
        QJsonArray pt = traps[i].toArray();
        d.trapPositions.append(QPoint(pt[0].toInt(), pt[1].toInt()));
    }

    // win animation
    d.hasWinAnimation = o.value("hasWinAnim").toBool();
    const QJsonArray wf = o.value("winFrame").toArray();
    for (int i = 0; i < wf.size(); ++i) {
        QJsonArray pt = wf[i].toArray();
        d.winFrameWords.append(QPoint(pt[0].toInt(), pt[1].toInt()));
    }

    return d;
}

//公开接口
LevelData createLevel1()  { return parseLevel(levelJson(1)); }
LevelData createLevel2()  { return parseLevel(levelJson(2)); }
LevelData createLevel3()  { return parseLevel(levelJson(3)); }
LevelData createLevel4()  { return parseLevel(levelJson(4)); }
LevelData createLevel5()  { return parseLevel(levelJson(5)); }
LevelData createLevel6()  { return parseLevel(levelJson(6)); }
LevelData createLevel7()  { return parseLevel(levelJson(7)); }
LevelData createLevel8()  { return parseLevel(levelJson(8)); }
LevelData createLevel9()  { return parseLevel(levelJson(9)); }
LevelData createLevel10() { return parseLevel(levelJson(10)); }
LevelData createLevel11() { return parseLevel(levelJson(11)); }
LevelData createLevel12() { return parseLevel(levelJson(12)); }
