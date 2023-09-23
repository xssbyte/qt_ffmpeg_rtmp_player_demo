#ifndef QIORINGBUFFER_H
#define QIORINGBUFFER_H

#include <QIODevice>
#include <QDebug>
#include <QAudioOutput>
/**
* @brief         QIORingBuffer class
* 继承QIODevice的环形缓冲区类
* 固定大小，无锁，无读写指针限制
* @author        Samuel<13321133339@163.com>
* @date          2023/09/23
*/
class QIORingBuffer : public QIODevice
{
    Q_OBJECT
public:
    QIORingBuffer(QObject *parent = nullptr)
        : QIODevice(parent), buffer(8192 * 16, '\0'), writePointer(0), readPointer(0){}
    QIORingBuffer(int buffer_size, QObject *parent = nullptr)
        : QIODevice(parent), buffer(buffer_size, '\0'), writePointer(0), readPointer(0){}
    qint64 readData(char *data, qint64 maxlen) override
    {
        qint64 bytesRead = qMin(maxlen, buffer.size() - readPointer);
        if (bytesRead > 0) {
            const QByteArray readData = buffer.mid(readPointer, bytesRead);
            memcpy(data, readData.constData(), bytesRead);
            readPointer = (readPointer + bytesRead) % buffer.size();
//            qDebug() << "readPointer" << readPointer;
            return bytesRead;
        }
        return 0;
    }

    qint64 writeData(const char *data, qint64 len) override
    {
        qint64 bytesWritten = qMin(len, buffer.size() - writePointer);
        if (bytesWritten > 0) {
            buffer.replace(writePointer, bytesWritten, data, bytesWritten);
            writePointer = (writePointer + bytesWritten) % buffer.size();
//            qDebug() << "writePointer" << writePointer;
            return bytesWritten;
        }
        return 0;
    }

private:
    QByteArray buffer;
    qint64 writePointer;
    qint64 readPointer;
};
#endif // QIORINGBUFFER_H
