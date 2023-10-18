#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->button_start_preview, &QAbstractButton::pressed, [&](){
        if(ui->tab_widget->currentIndex() == ui->tab_widget->indexOf(ui->player_widget))
            ui->player_widget->start_preview(ui->url_line_edit->text().toStdString());
        else if(ui->tab_widget->currentIndex() == ui->tab_widget->indexOf(ui->gl_player_widget))
            ui->gl_player_widget->start_preview(ui->url_line_edit->text().toStdString());
//        ui->player_widget->start_preview("rtsp://127.0.0.1:554/live/stream01");
    });
    QObject::connect(ui->button_stop_preview, &QAbstractButton::pressed, [&](){
        if(ui->tab_widget->currentIndex() == ui->tab_widget->indexOf(ui->player_widget))
            ui->player_widget->stop_preview();
        else if(ui->tab_widget->currentIndex() == ui->tab_widget->indexOf(ui->gl_player_widget))
            ui->gl_player_widget->stop_preview();
    });
    QObject::connect(ui->button_start_record, &QAbstractButton::pressed, [&](){
        QDateTime currentDateTime = QDateTime::currentDateTime();
        QString fileName = currentDateTime.toString("yyyy-MM-dd_hhmmss") + ".mp4";
        if(ui->tab_widget->currentIndex() == ui->tab_widget->indexOf(ui->player_widget))
            ui->player_widget->start_local_record(fileName.toStdString());
        else if(ui->tab_widget->currentIndex() == ui->tab_widget->indexOf(ui->gl_player_widget))
            ui->gl_player_widget->start_local_record(fileName.toStdString());
    });
    QObject::connect(ui->button_stop_record, &QAbstractButton::pressed, [&](){
        if(ui->tab_widget->currentIndex() == ui->tab_widget->indexOf(ui->player_widget))
            ui->player_widget->stop_local_record();
        else if(ui->tab_widget->currentIndex() == ui->tab_widget->indexOf(ui->gl_player_widget))
            ui->gl_player_widget->stop_local_record();
    });
    QObject::connect(ui->tab_widget, &QTabWidget::currentChanged, [&](int index){
        if((index == ui->tab_widget->indexOf(ui->player_widget))
                || (index == ui->tab_widget->indexOf(ui->gl_player_widget)))
        {
            ui->player_widget->stop_preview();
            ui->gl_player_widget->stop_preview();
        }
        else
        {
            assert(0);
        }
    });
    ui->url_line_edit->setPlaceholderText("rtsp://192.168.1.160:8554/0");
    ui->tab_widget->setCurrentWidget(ui->player_widget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

