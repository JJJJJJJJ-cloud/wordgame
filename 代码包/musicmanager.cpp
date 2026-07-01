#include "musicmanager.h"

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

// 文件映射：index 、 文件名
static const QMap<int, QString> kMusicFilenames = {
    {0,  QStringLiteral("level0_mainmenu.mp3")},
    {1,  QStringLiteral("level1.mp3")},
    {2,  QStringLiteral("level2.mp3")},
    {3,  QStringLiteral("level3.mp3")},
    {4,  QStringLiteral("level4.mp3")},
    {5,  QStringLiteral("level5.mp3")},
    {6,  QStringLiteral("level6.mp3")},
    {7,  QStringLiteral("level7.mp3")},
    {8,  QStringLiteral("level8.mp3")},
    {9,  QStringLiteral("level9_bodleasons-dramatic-trailer-292423.mp3")},
    {10, QStringLiteral("level10_nastelbom-epic-fight-436865.mp3")},
    {11, QStringLiteral("level11_leberch-sad-516351.mp3")},
    {12, QStringLiteral("level12_the_mountain-ancient-empire-142301 (1).mp3")},
    {13, QStringLiteral("level13_newmainmenu_musicword-end-240220.mp3")},
};

// 构造 / 析构
MusicManager::MusicManager(QObject *parent)
    : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    // 应用默认音量
    m_audioOutput->setVolume(m_volumeSetting / 100.0);

    // 淡入淡出定时器
    m_fadeTimer = new QTimer(this);
    m_fadeTimer->setInterval(30);  // ~30ms per step = ~33 fps

    // 运行时错误处理
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error err, const QString &errStr) {
        Q_UNUSED(err)
        qWarning("MusicManager: player error — %s", qPrintable(errStr));
        abortFade();
    });

    // 解析 myaudio 文件夹路径
    m_baseDir = resolveBaseDir();
    qDebug("MusicManager: base directory = %s", qPrintable(m_baseDir));
}

MusicManager::~MusicManager()
{
    abortFade();
    m_player->stop();
}

// 公共 API
void MusicManager::playMenuMusic()
{
    QString path = filePathForIndex(0);
    playFile(path);
}

void MusicManager::playPostGameMenuMusic()
{
    QString path = filePathForIndex(13);
    playFile(path);
}

void MusicManager::playLevelMusic(int level)
{
    if (level < 1 || level > 12) return;
    QString path = filePathForIndex(level);
    playFile(path);
}

void MusicManager::stopMusic()
{
    abortFade();
    m_player->stop();
}

void MusicManager::setVolume(int percent)
{
    m_volumeSetting = qBound(0, percent, 100);
    if (!m_isFading)
        m_audioOutput->setVolume(m_volumeSetting / 100.0);
    // 如果在淡入淡出中，目标音量在 fade 结束后生效
}

// 内部：播放文件
void MusicManager::playFile(const QString &filePath, std::function<void()> onStarted)
{
    // 防止重复播放同一文件
    if (m_player->playbackState() == QMediaPlayer::PlayingState
        && m_player->source().toLocalFile() == filePath
        && !m_isFading) {
        if (onStarted) onStarted();
        return;
    }

    // 如果正在淡出，中断它
    if (m_isFading && m_fadeTargetVol == 0.0) {
        abortFade();
    }

    // 文件存在检查
    if (!QFile::exists(filePath)) {
        qWarning("MusicManager: file not found — %s", qPrintable(filePath));
        if (onStarted) onStarted();
        return;
    }

    // 如果当前有音乐在播放，先淡出再加载新曲
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        startFadeOut(2000, [this, filePath, onStarted]() {
            m_player->stop();
            m_player->setSource(QUrl::fromLocalFile(filePath));
            m_audioOutput->setVolume(0.0);
            m_player->play();
            if (onStarted) onStarted();
            startFadeIn(5000);
        });
    } else {
        m_player->stop();
        m_player->setSource(QUrl::fromLocalFile(filePath));
        m_audioOutput->setVolume(0.0);
        m_player->play();
        if (onStarted) onStarted();
        startFadeIn(5000);
    }
}

// 淡入
void MusicManager::startFadeIn(int durationMs)
{
    abortFade();

    m_isFading = true;
    m_fadeTotalSteps = durationMs / 30;          // 例如 5000/30 ≈ 167
    if (m_fadeTotalSteps < 1) m_fadeTotalSteps = 1;
    m_fadeStepCount = 0;
    m_fadeStartVol = 0.0;
    m_fadeTargetVol = m_volumeSetting / 100.0;
    m_fadeDoneCb = nullptr;

    connect(m_fadeTimer, &QTimer::timeout, this, [this]() {
        m_fadeStepCount++;
        double t = (double)m_fadeStepCount / m_fadeTotalSteps;
        if (t >= 1.0) {
            t = 1.0;
            m_isFading = false;
            m_fadeTimer->stop();
            disconnect(m_fadeTimer, nullptr, this, nullptr);
            if (m_fadeDoneCb) { auto cb = m_fadeDoneCb; m_fadeDoneCb = nullptr; cb(); }
        }
        // 线性插值
        double vol = m_fadeStartVol + (m_fadeTargetVol - m_fadeStartVol) * t;
        m_audioOutput->setVolume(vol);
    });

    m_fadeTimer->start();
}

// 淡出
void MusicManager::startFadeOut(int durationMs, std::function<void()> onDone)
{
    abortFade();

    m_isFading = true;
    m_fadeTotalSteps = durationMs / 30;
    if (m_fadeTotalSteps < 1) m_fadeTotalSteps = 1;
    m_fadeStepCount = 0;
    m_fadeStartVol = m_audioOutput->volume();
    m_fadeTargetVol = 0.0;
    m_fadeDoneCb = std::move(onDone);

    connect(m_fadeTimer, &QTimer::timeout, this, [this]() {
        m_fadeStepCount++;
        double t = (double)m_fadeStepCount / m_fadeTotalSteps;
        if (t >= 1.0) {
            t = 1.0;
            m_isFading = false;
            m_fadeTimer->stop();
            disconnect(m_fadeTimer, nullptr, this, nullptr);
            m_audioOutput->setVolume(0.0);
            if (m_fadeDoneCb) { auto cb = m_fadeDoneCb; m_fadeDoneCb = nullptr; cb(); }
            return;
        }
        double vol = m_fadeStartVol + (m_fadeTargetVol - m_fadeStartVol) * t;
        m_audioOutput->setVolume(vol);
    });

    m_fadeTimer->start();
}

// 中断淡入淡出
void MusicManager::abortFade()
{
    if (m_fadeTimer->isActive())
        m_fadeTimer->stop();
    disconnect(m_fadeTimer, nullptr, this, nullptr);
    m_isFading = false;
    m_fadeDoneCb = nullptr;
}

// 文件路径
QString MusicManager::filePathForIndex(int index) const
{
    if (!kMusicFilenames.contains(index))
        return {};
    return m_baseDir + kMusicFilenames.value(index);
}

QString MusicManager::resolveBaseDir() const
{
    // 策略1：exe 同级目录下的 myaudio/
    QString dir = QCoreApplication::applicationDirPath() + "/myaudio";
    if (QDir(dir).exists()) return dir + "/";

    // 策略2：从构建目录向上一级（Qt Creator debug 构建目录结构）
    QDir buildDir(QCoreApplication::applicationDirPath());
    if (buildDir.dirName() == "debug" || buildDir.dirName().contains("Desktop_")) {
        QString srcDir = buildDir.absolutePath() + "/../../myaudio";
        if (QDir(srcDir).exists()) return QDir(srcDir).absolutePath() + "/";
    }

    // 策略3：当前工作目录
    QString cwd = QDir::currentPath() + "/myaudio";
    if (QDir(cwd).exists()) return cwd + "/";

    // 回退：返回 exe 目录（即使不存在）
    qWarning("MusicManager: myaudio folder not found, music disabled");
    return QCoreApplication::applicationDirPath() + "/myaudio/";
}
