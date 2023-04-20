#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <QDebug>
#include <QThread>


VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent), m_player(nullptr)
{
    // Set size and background color
    setFixedSize(800, 600);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(palette);


    QThread* m_playerThread = new QThread;
    // Create player
    m_player = new RTMPPlayer(this, nullptr);
    m_player->moveToThread(m_playerThread);
    connect(m_player,&RTMPPlayer::sigUpdateUI,this,QOverload<>::of(&VideoWidget::update));
    connect(m_player, &RTMPPlayer::error, [](const QString &msg) {
        qWarning() << "Error:" << msg;
    });

    m_playerThread->start();
    //+Q_OBJECT
    //connect(this, &VideoWidget::sigStartToPlay,m_player ,&RTMPPlayer::start);
    QMetaObject::invokeMethod(m_player, "start", Qt::QueuedConnection, Q_ARG(QString, "rtmp://192.168.1.180:8080/live/A60010C221062342"));
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    if (m_player && m_player->m_started) {
        // Draw image
        QPainter painter(this);
        QImage m_image;
        m_player->getImage(m_image);
        painter.drawImage(rect(), m_image);
    }
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug() << "FFmpeg version: " << av_version_info();
    VideoWidget* mVideoWidget = new VideoWidget();
    mVideoWidget->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

