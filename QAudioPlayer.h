#ifndef QAUDIOPLAYER_H
#define QAUDIOPLAYER_H

#include <memory>
#include <mutex>

#include <QIORingBuffer.h>
#include <QAudioOutput>

/**
* @brief          QAudioPlayer class
* 音频播放类,音频缓冲区大小需要根据实际情况修改，一般是帧大小的整数倍
* 缓冲区大延时高稳定性好，缓冲区小实时性好稳定性差
* @author        Samuel<13321133339@163.com>
* @date          2023/09/23
*/

class QAudioPlayer : public QIORingBuffer
{
    Q_OBJECT
public:
    QAudioPlayer() = default;

    QAudioPlayer(int buffer_size);
    ~QAudioPlayer();

    void start_consume_audio();
    void stop_consume_audio();
private:
    std::unique_ptr<QAudioOutput> m_audioOutput;
    std::mutex _mutex;
};

#endif // QAUDIOPLAYER_H
