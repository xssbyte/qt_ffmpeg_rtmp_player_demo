#include <QAudioPlayer.h>

QAudioPlayer::QAudioPlayer(int buffer_size)
    :QIORingBuffer(buffer_size)
{
    // 配置音频设备参数
    QAudioFormat format;
    format.setSampleRate(48000); // 采样率
    format.setChannelCount(2);  // 声道数
    format.setSampleSize(16);    // 采样位数
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);
    // 创建QAudioOutput对象
    m_audioOutput.reset(new QAudioOutput(format));
    m_audioOutput->setVolume(100);
    if(!this->open(QIODevice::ReadWrite | QIODevice::Unbuffered))
    {
        qCritical() << "Failed open QAudioPlayer";
    };
}

QAudioPlayer::~QAudioPlayer()
{
    m_audioOutput->stop();
}
void QAudioPlayer::start_consume_audio()
{
    std::lock_guard<std::mutex> _l(_mutex);
    if(m_audioOutput->state() == QAudio::StoppedState)
    {
        m_audioOutput->start(this);
    }
}
void QAudioPlayer::stop_consume_audio()
{
    std::lock_guard<std::mutex> _l(_mutex);
    if(m_audioOutput->state() == QAudio::ActiveState)
    {
        m_audioOutput->stop();
    }
}
