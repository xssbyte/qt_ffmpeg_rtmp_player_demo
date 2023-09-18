# qt_ffmpeg_rtmp_player_demo
qt+ffmpeg+opengl实现rtmp/rtsp流播放器，只有播放和停止，功能简单，代码简单，性能优秀，生产消费无锁，无额外拷贝。  
## 架构
单生产者单消费者模型，使用原子量做同步，生产消费过程无锁，只有需要消费才拷贝。  
低耦合，解码只使用stl库和ffmpeg库，UI只使用stl库和qt库，无其他依赖项。  
## 生产者producer
ffmpeg的c接口实现拉流，解码，格式为AV_PIX_FMT_RGBA  
## 消费者consumer
实现了两种方案，QWidget异步绘制和QOpenGLWidget同步绘制  
QWidget方案，有一次额外深拷贝。这是为了保证qt框架异步消费时，图像还没有被析构。paintEvent到实际上屏的过程也是异步的，QPainter只是指向了传入的图像。如果paintEvent后马上析构图像，到qt框架异步消费时，正常还会有一次深拷贝，但是指向的图像已经析构了，就会造成垂悬指针。对于普通QWidget，qt框架没有提供同步消费的方案，因为如果同步消费回调里阻塞了，主线程就阻塞了，就会导致程序无响应。  
QOpenGLWidget方案，没有额外的深拷贝，paintGL回调中调用的opengl函数都是同步函数，glTexSubImage2D传入纹理指针后，会被opengl深拷贝，之后就可以释放图像数据指针了。  
## 同步和帧率控制
单生产者单消费者，使用一个原子量做同步，更复杂的模型需要独立实现生产者消费者队列，超出demo的范围了。  
帧率控制没有特意实现，update()有自动去重，所以只是利用了每一帧的回调，正常来说，控制取帧和抽帧频率，使用QTimer，在定时器中调用update()触发重绘即可。  
每一帧的回调是用于完全同步消费的，可以传入AVFrame指针并同步消费。qt框架不支持所以没有实现。  
## TODO
音频，音视频同步，画面缩放。
## 依赖和编译
需要修改pro文件中的ffmpeg库的库和头文件的路径  
直接build，注意64位还是32位，并把ffmpeg的库拷贝到build出来的文件夹中  
![img](https://github.com/xssbyte/qt_ffmpeg_rtmp_player_demo/blob/master/res/qt_ffmpeg_rtmp_player.png)  
