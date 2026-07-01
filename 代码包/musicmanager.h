#ifndef MUSICMANAGER_H
#define MUSICMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <functional>

class QMediaPlayer;
class QAudioOutput;
class QTimer;

class MusicManager : public QObject
{
    Q_OBJECT
public:
    explicit MusicManager(QObject *parent = nullptr);
    ~MusicManager();

    // 播放控制
    void playMenuMusic();              // 播放原版菜单音乐（level0）
    void playPostGameMenuMusic();      // 播放通关后菜单音乐（level13）
    void playLevelMusic(int level);    // 播放关卡音乐（level 1-12）
    void stopMusic();                  // 立即停止（无淡出）

    // 音量控制
    void setVolume(int percent);       // 0-100
    int volume() const { return m_volumeSetting; }

private:
    void playFile(const QString &filePath, std::function<void()> onStarted = nullptr);
    void startFadeIn(int durationMs = 5000);
    void startFadeOut(int durationMs, std::function<void()> onDone);
    void abortFade();
    QString filePathForIndex(int index) const;
    QString resolveBaseDir() const;

    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    QTimer *m_fadeTimer;

    // 淡入淡出状态
    bool m_isFading = false;
    int m_fadeStepCount = 0;
    int m_fadeTotalSteps = 0;
    double m_fadeStartVol = 0.0;
    double m_fadeTargetVol = 0.0;
    std::function<void()> m_fadeDoneCb;

    int m_volumeSetting = 50;          // 用户设置 0-100
    QString m_baseDir;                 // myaudio 文件夹路径
    QString m_pendingFile;             // 等待淡出完成后播放的文件
};

#endif // MUSICMANAGER_H
