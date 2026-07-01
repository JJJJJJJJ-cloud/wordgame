#ifndef PROLOGUEWIDGET_H
#define PROLOGUEWIDGET_H

#include <QWidget>
#include <QTimer>

class PrologueWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PrologueWidget(QWidget *parent = nullptr);

    void start();
    void stop();
    void setPostGameMode(bool on) { m_isPostGame = on; }

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QStringList m_lines;
    int m_currentLine = 0;
    int m_currentCol = 0;
    bool m_active = false;

    QTimer *m_typeTimer;
    QTimer *m_cursorTimer;
    QTimer *m_finishTimer;
    bool m_cursorVisible = true;
    bool m_isPostGame = false;

    static constexpr int kFontSize = 24;
    static constexpr int kStartX = 2;
    static constexpr int kStartY = 7;
    static constexpr int kGridSize = 40;
};

#endif // PROLOGUEWIDGET_H
