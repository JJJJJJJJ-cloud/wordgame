QT       += core gui widgets multimedia
TARGET = WASD_Demo
TEMPLATE = app
SOURCES += main.cpp \
           levelbase.cpp \
           level1.cpp \
           level2.cpp \
           musicmanager.cpp \
           mainmenu.cpp \
           prologuewidget.cpp \
           mainwindow.cpp \
           gameview.cpp \
           gamescene.cpp \
           leveldata.cpp \
           typewriter.cpp \
           dialoguesystem.cpp \
           pausemenu.cpp \
           level3road.cpp \
           level4classroom.cpp \
           fishingmechanic.cpp \
           platformermechanic.cpp \
           level7.cpp \
           deleteeffect.cpp \
           deatheffect.cpp \
           interactionsystem.cpp \
           level8.cpp \
           level9.cpp \
           level10.cpp \
           level11.cpp \
           level12.cpp \
           creditswidget.cpp \
           achievement.cpp
HEADERS += musicmanager.h \
           levelbase.h \
           level1.h \
           level2.h \
           mainwindow.h \
           gameview.h \
           gamescene.h \
           mainmenu.h \
           prologuewidget.h \
           grid.h \
           leveldata.h \
           typewriter.h \
           savemanager.h \
           dialoguesystem.h \
           pausemenu.h \
           level3road.h \
           level4classroom.h \
           fishingmechanic.h \
           platformermechanic.h \
           level7.h \
           deleteeffect.h \
           deatheffect.h \
           interactionsystem.h \
           level8.h \
           level9.h \
           level10.h \
           level11.h \
           level12.h \
           creditswidget.h \
           achievement.h \
           animutils.h

# 构建后将资源文件复制到输出目录
win32 {
    copydata.commands = $$quote(if not exist \"$$OUT_PWD\\myaudio\" mkdir \"$$OUT_PWD\\myaudio\") && $$quote(xcopy /Y /D \"$$PWD\\myaudio\\*.mp3\" \"$$OUT_PWD\\myaudio\\\")
# levels.json is now embedded in leveldata.cpp — no copy needed
} else {
    copydata.commands = $$quote($(MKDIR) \"$$OUT_PWD/myaudio\" 2>/dev/null; cp -u \"$$PWD/myaudio/\"*.mp3 \"$$OUT_PWD/myaudio/\" 2>/dev/null)
}
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata
