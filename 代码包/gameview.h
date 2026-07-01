#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QGraphicsView>
#include <QTimer>
#include <QSet>
#include <QResizeEvent>   // 必须包含，否则 resizeEvent 声明报错

class GameView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit GameView(QWidget *parent = nullptr);

    void setZoomLocked(bool locked) { m_zoomLocked = locked; }
    bool isZoomLocked() const { return m_zoomLocked; }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void processMovement();

private:
    QTimer *m_moveTimer;
    QSet<int> m_keysHeld;
    bool m_zoomLocked = false;
};

#endif