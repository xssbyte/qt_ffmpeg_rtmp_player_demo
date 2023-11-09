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
#include <QTimer>

#include <FFmpegPlayer.h>
#include <QAudioPlayer.h>
/**
* @brief         QPlayerWidget class
* QPlayerWidget继承QWidget，使用qt框架的UI更新事件。先转为QImage，然后通过调用update()实现刷新上屏
* @author        Samuel<13321133339@163.com>
* @date          2023/09/23
*/
class QPlayerWidget : public QWidget, public FFmpegPlayer
{
    Q_OBJECT
public:
    QPlayerWidget(QWidget *parent = nullptr);
public slots:
    void start_preview(const std::string &media_url);
    void stop_preview();
    void start_local_record(const std::string &output_file);
    void stop_local_record();

protected:
    void paintEvent(QPaintEvent *event) override;

    void on_new_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache) override;
    void on_new_audio_frame_avaliable(std::shared_ptr<FrameCache> m_frame_cache) override;
private:
    QImage m_image;
    std::unique_ptr<QAudioPlayer> m_audioPlayer;
};

#endif // QPLAYERWIDGET_H
