#ifndef QIORINGBUFFER_H
#define QIORINGBUFFER_H

#include <QIODevice>
#include <QDebug>

class QIORingBuffer : public QIODevice
{
    Q_OBJECT
public:
    QIORingBuffer(QObject *parent = nullptr)
        : QIODevice(parent)
    {
        buffer.resize(8192 * 8);
        writePointer = 0;
        readPointer = 0;
    }
    qint64 readData(char *data, qint64 maxlen) override
    {
        qint64 bytesRead = qMin(maxlen, buffer.size() - readPointer);
        if (bytesRead > 0) {
            const QByteArray readData = buffer.mid(readPointer, bytesRead);
            memcpy(data, readData.constData(), bytesRead);
            readPointer = (readPointer + bytesRead) % buffer.size();
            qDebug() << "readPointer" << readPointer;
            return bytesRead;
        }

        return 1;
    }

    qint64 writeData(const char *data, qint64 len) override
    {
        qint64 bytesWritten = qMin(len, buffer.size() - writePointer);
        if (bytesWritten > 0) {
            buffer.replace(writePointer, bytesWritten, data, bytesWritten);
            writePointer = (writePointer + bytesWritten) % buffer.size();
            qDebug() << "writePointer" << writePointer;
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
