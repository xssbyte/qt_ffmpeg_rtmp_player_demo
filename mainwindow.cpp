#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->button_start_preview,&QAbstractButton::pressed,ui->player_widget,[&](){
        ui->player_widget->start_preview("rtsp://43.143.167.140:554/live/stream01");
    });
    QObject::connect(ui->button_stop_preview,&QAbstractButton::pressed,ui->player_widget,[&](){
        ui->player_widget->stop_preview();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

