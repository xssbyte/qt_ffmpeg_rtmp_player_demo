#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <FFmpegPlayer.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
/**
 * @brief The QPlayerWidget class
 * 继承QWidget，使用qt框架的UI更新事件。先转为QImage，然后通过调用update()实现刷新上屏
 * 这种做法实现最简单，对事件循环压力比较大，每一帧都需要同步更新UI，帧率等于视频帧率
 * 不适合预览，但是适合做取帧和抽帧
 * 对于预览，最好的方案是使用ffmpeg+opengl主动控制上屏，不依赖于qt的事件循环
 */

class QPlayerWidget : public QWidget
{
    Q_OBJECT
public:
    QPlayerWidget(QWidget *parent = nullptr);
public slots:
    void start_preview(const std::string &media_url);
    void stop_preview();
    void set_preview_callback(std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> callback);
protected:
    void paintEvent(QPaintEvent *event);
private:
    std::function<void (uint8_t*/*data*/,int/*w*/,int/*h*/)> preview_callback;
    std::unique_ptr<FFmpegPlayer> p_ffmpeg_player;
    std::atomic_bool frame_avaliable = false;
    QImage m_image;
private:
    Ui::MainWindow *ui;
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
