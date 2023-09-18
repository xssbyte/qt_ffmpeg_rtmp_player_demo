#ifndef QAUDIOPLAYER_H
#define QAUDIOPLAYER_H
/**
* @brief         音频播放类
* @author        Samuel<13321133339@163.com>
* @date          2023/09/18
*/

#include <QIODevice>
#include <QDebug>
#include <QAudioOutput>

class AudioDevice : public QIODevice
{
public:
    AudioDevice(QObject* parent = nullptr) : QIODevice(parent) {
        open(QIODevice::WriteOnly);
    }

    qint64 writeData(const char* data, qint64 len) override {
        // 在这里处理音频数据（data），您可以在这里添加音频帧
        // 这个示例只是简单地输出音频数据的长度
        qDebug() << "Received audio data of length:" << len;
        return len;
    }
};


#endif // QAUDIOPLAYER_H
