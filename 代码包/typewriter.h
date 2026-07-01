#ifndef TYPEWRITER_H
#define TYPEWRITER_H

#include <QObject>
#include <QTimer>
#include <QPointer>
#include <QGraphicsTextItem>

class TypewriterEffect : public QObject
{
    Q_OBJECT
public:
    explicit TypewriterEffect(QObject *parent = nullptr);

    void start(QGraphicsTextItem *item, const QString &fullText, int intervalMs = 60);
    void skip();
    bool isRunning() const { return m_running; }

signals:
    void finished();

private:
    QTimer *m_timer;
    QPointer<QGraphicsTextItem> m_item;
    QString m_fullText;
    int m_index = 0;
    bool m_running = false;
};

#endif
