#ifndef RTMPPLAYER_H
#define RTMPPLAYER_H
#include <QTimer>
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QWidget>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class RTMPPlayer : public QObject
{
    Q_OBJECT
public:
    explicit RTMPPlayer(QWidget *widget, QObject *parent = nullptr);
    ~RTMPPlayer();
public:
    std::atomic_bool m_started;
    void restart();
    void getImage(QImage& image);
public slots:
    void start(const QString &url);
    void stop();

signals:
    void sigUpdateUI();
    void error(const QString &msg);

private slots:
    void updateFrame();

private:
    void cleanup();
    void reconnect();

    QWidget *m_widget;
    AVFormatContext *m_formatCtx;
    AVCodecContext *m_codecCtx;
    AVDictionary *m_options;
    AVFrame *m_frame;
    AVPacket *m_packet;

    std::unique_ptr<QTimer> m_timer;
    QImage m_image;
    SwsContext *m_swsCtx;
    QString m_url;
    int m_videoStream;

};
#endif // RTMPPLAYER_H
