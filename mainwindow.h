#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <FFmpegPlayer.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPlayerWidget : public QWidget
{
public:
    QPlayerWidget(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event);
private:
    std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> preview_callback;
    std::unique_ptr<FFmpegPlayer> p_ffmpeg_player;
    QImage m_image;
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
