#ifndef LEVELDATA_H
#define LEVELDATA_H

#include <QString>
#include <QList>
#include <QPoint>
#include <QColor>

struct LevelItem {
    QString text;
    int gx, gy;
    enum Type { Player, Pushable, Water, Scenery, Special, Trap };
    Type type;
    int rotation = 0;         // 角度（度），默认不旋转
    QColor customColor;       // 自定义颜色（无效颜色 = 使用默认白色）
};

struct DecorItem {
    QString text;
    int gx = 0, gy = 0;
    bool blocking = true;
    bool swaying = false;
    QColor color;
};

struct ComboRule {
    QString pushableText;
    QString targetText;
    QString resultText;
    int adjacencyDir = 0;
    int autoWalkX = -1, autoWalkY = -1;
};

struct PhraseCheckRule {
    QList<QPoint> positions;
    QString winPhrase;
    QString badPhrase;
};

struct LevelData {
    int levelNumber = 0;

    enum Mechanic {
        Standard,
        TextCombine,
        PhraseReplace,
        PhraseRearrange,
        PureNarrative,
        Fishing,
        TowerClimb
    };
    Mechanic mechanic = Standard;

    enum WinCondition { HasText, NoText };
    WinCondition winCondition = HasText;
    QString winTarget;

    QList<LevelItem> items;

    // 场景布景（带可选摆动动画）
    QList<DecorItem> decors;

    QList<ComboRule> comboRules;
    QList<PhraseCheckRule> phraseRules;

    QList<QPoint> fadeOutPositions;
    bool hasFadeOutSequence = false;

    QStringList dialogues;
    QString winDialogue;

    QString tutorialHintText;
    int hintGx = -1, hintGy = -1;

    bool hasWinAnimation = false;
    QList<QPoint> winFrameWords;

    QList<QPoint> trapPositions;
    int climbWinX = -1, climbWinY = -1;

    int fishingRopeStartX = -1;
    QStringList fishingItems;
    int fishingTargetIndex = -1;
};

LevelData createLevel1();
LevelData createLevel2();
LevelData createLevel3();
LevelData createLevel4();
LevelData createLevel5();
LevelData createLevel6();
LevelData createLevel7();
LevelData createLevel8();
LevelData createLevel9();
LevelData createLevel10();
LevelData createLevel11();
LevelData createLevel12();

#endif
