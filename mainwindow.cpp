#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <QDebug>
#include <QThread>
#include <QPainter>

QPlayerWidget::QPlayerWidget(QWidget *parent) : QWidget(parent),
    p_ffmpeg_player(new FFmpegPlayer()), ui(new Ui::MainWindow)
{
    // Set size and background color
    setFixedSize(640, 480);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(palette);

    p_ffmpeg_player->set_preview_callback([&](uint8_t *data,int width,int height){
        qDebug() << "preview_callback";
        if(!frame_avaliable.load())
        {
            QImage frame_image(data, width, height, QImage::Format_RGBA8888);
            m_image = frame_image.copy();
            this->update();
        }
        frame_avaliable.store(true);
    });
}
void QPlayerWidget::paintEvent(QPaintEvent *event)
{
    qDebug() << __FUNCTION__;
    if(frame_avaliable.load())
    {
        if (p_ffmpeg_player->m_started) {
            QPainter painter(this);
            painter.drawImage(rect(), m_image);
        }
    }
    frame_avaliable.store(false);
}
void QPlayerWidget::start_preview(const std::string &media_url)
{
    p_ffmpeg_player->start_preview(media_url);
}
void QPlayerWidget::stop_preview()
{
    p_ffmpeg_player->stop_preview();
}
void QPlayerWidget::set_preview_callback(std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> callback)
{
    p_ffmpeg_player->set_preview_callback(callback);
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug() << "FFmpeg version: " << av_version_info();
    QObject::connect(ui->button_start_preview,&QAbstractButton::pressed,ui->player_widget,[&](){
        ui->player_widget->start_preview("rtsp://192.168.1.106:554/live/stream");
    });
    QObject::connect(ui->button_stop_preview,&QAbstractButton::pressed,ui->player_widget,[&](){
        ui->player_widget->stop_preview();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

