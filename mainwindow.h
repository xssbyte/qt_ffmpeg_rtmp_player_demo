#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <rtmpPlayer.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class VideoWidget : public QWidget
{
public:
    VideoWidget(QWidget *parent = nullptr);
    void sigStartToPlay(const QString& url);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    RTMPPlayer *m_player;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
