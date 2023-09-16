#ifndef QPLAYERWIDGET_H
#define QPLAYERWIDGET_H

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <QDebug>
#include <QThread>
#include <QPainter>
#include <QWidget>

#include <FFmpegPlayer.h>

/**
 * @brief The QPlayerWidget class
 * Qt对于普通QWidget，只能使用异步绘制到主线程，缓存一帧至少有一次memcpy，如果要实现同步绘制，只能使用QOpenGLWidget
 * QPlayerWidget继承QWidget，使用qt框架的UI更新事件。先转为QImage，然后通过调用update()实现刷新上屏
 * 实现简单，有一次拷贝QImage的开销，对事件循环压力大，不适合预览，但是适合做取帧和抽帧
 */

class QPlayerWidget : public QWidget, protected FFmpegPlayer
{
    Q_OBJECT
public:
    QPlayerWidget(QWidget *parent = nullptr);
public slots:
    void start_preview(const std::string &media_url);
    void stop_preview();
protected:
    void paintEvent(QPaintEvent *event) override;

    void on_new_frame_avaliable() override;
private:
    QImage m_image;
};

#endif // QPLAYERWIDGET_H
