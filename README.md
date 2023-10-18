# qt_ffmpeg_rtmp_player_demo
qt+ffmpeg+opengl实现rtmp/rtsp音视频流播放器，功能有播放停止音视频流，同时录制视频流。功能简单，代码简单，性能优秀。  
优先保证实时性，不做音视频帧同步，适合对延时要求非常高的场景，比如rtsp摄像头拉流预览和录制。  
## 架构
单生产者单消费者模型，视频帧使用原子量做同步，生产消费过程无锁，只有需要消费才拷贝。  
对于播放器，实时性和高性能是冲突的，实时就要逐帧处理但只取最新帧消费，高性能就要求不能有额外处理，上屏几帧就处理几帧，所以一般使用折衷的做法。  
消费者消费完上一帧了，消费者才处理下一帧，生产者主动触发消费，消费者不会比生产者更快，所以不用缓存，如果使用QTimer触发主动消费，频率不一致就需要对是否被消费做检查，或者实现额外的缓冲区。  
音频帧生产消费使用环形缓冲区实现，自己实现缓冲区并逐帧拷贝。音频消费者为qt框架的情况，我没有更简单的方案了。  
低耦合，解码只使用c++基础库和ffmpeg库，生产者部分不依赖于qt框架。UI使用c++基础库和qt库，没有其他依赖项了。  
## 视频生产者video producer:
ffmpeg读取视频流+软解码+色彩空间转换，从原始rtsp流格式(一般是AV_PIX_FMT_YUV420P)统一转成AV_PIX_FMT_RGBA  
## 视频消费者video consumer:
实现了两种方案，QWidget异步绘制和QOpenGLWidget同步绘制  
QWidget方案，有一次额外深拷贝。这是为了保证qt框架异步消费时，图像还没有被析构。paintEvent到实际上屏的过程也是异步的，QPainter只是指向了传入的图像。如果paintEvent后马上析构图像，到qt框架异步消费时，正常还会有一次深拷贝，但是指向的图像已经析构了，就会造成垂悬指针。对于普通QWidget，qt框架没有提供同步消费的方案，update()函数涉及的步骤太多了，感兴趣可以看看qt的源码。  
QOpenGLWidget方案，没有额外的深拷贝，paintGL回调中调用的opengl函数都是同步函数，glTexSubImage2D传入纹理指针后，会被opengl深拷贝，之后释放图像数据指针就不会有垂悬指针的问题。  
## 音频生产者audio producer:
ffmpeg读取音频流+软解码+音频重采样，重采样为pcm格式  
## 音频消费者audio consumer:
QAudioOutput在open时，如果未设置QIODevice::Unbuffered，实时性很差，所以demo里对音频QIODevice禁用了qt内部缓冲。实现了一个环形缓冲区，缓存和实时播放音频帧，保证QAudioOutput始终有数据可读。QAudioOutput循环读环形缓冲区，ffmpeg循环写入环形缓冲区。格式一致的情况下播放延时是固定的，和环形缓冲区大小相关，缓冲区大延时高稳定性好，缓冲区小实时性好稳定性差，可以根据需求调整缓冲区大小。  
## 同步和帧率控制
视频帧是单生产者单消费者，使用一个原子量做同步，更复杂的模型需要独立实现生产者消费者队列，超出demo的范围了。  
帧率控制没有特意实现，update()有自动去重，只是利用了每一帧的回调触发重绘，如果需要按频率取帧，不需要显示，可以使用Qtimer触发，并维护一个缓冲区。
每一帧的回调on_new_frame_avaliable和on_new_audio_frame_avaliable是用于同步消费或者做音视频同步或者存入缓冲区的。音视频同步不在demo的范围内了。  
## 录制功能
当视频流可用时，可以调用开始录制和停止录制的方法实时录制保存为mp4文件，可以同时开始录制和播放，同时停止录制和播放。  
这只是一个示例，所有取流的feature都可以以类似的方式实现。  
## TODO
视频画面缩放，音频播放流程优化，QTimer取帧的demo  
## 依赖和编译
需要修改pro文件中的ffmpeg库的库和头文件的路径  
直接build，注意64位还是32位，并把ffmpeg的库拷贝到build出来的文件夹中  
![img](https://github.com/xssbyte/qt_ffmpeg_rtmp_player_demo/blob/master/res/qt_ffmpeg_rtmp_player.png)  
延时效果
![img](https://github.com/xssbyte/qt_ffmpeg_rtmp_player_demo/blob/master/res/qt_ffmpeg_rtmp_player.gif)  