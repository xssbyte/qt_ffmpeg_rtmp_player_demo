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

QPlayerWidget::QPlayerWidget(QWidget *parent) :
    p_ffmpeg_player(new FFmpegPlayer())
{
    // Set size and background color
    setFixedSize(800, 600);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(palette);

    p_ffmpeg_player->set_preview_callback([&](uint8_t *data,int width,int height){
        QImage frame_image(data, width, height, QImage::Format_RGBA8888);
        m_image = frame_image.copy();
        this->update();
        qDebug() << "preview_callback";
    });
    p_ffmpeg_player->start_preview("rtsp://admin:12345@192.168.1.7:8554/0");
}
void QPlayerWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (p_ffmpeg_player->m_started) {
        QPainter painter(this);
        painter.drawImage(rect(), m_image);
        qDebug() << __FUNCTION__;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug() << "FFmpeg version: " << av_version_info();
    QPlayerWidget *mVideoWidget = new QPlayerWidget();
    mVideoWidget->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

