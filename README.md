# qt_ffmpeg_rtmp_player
![img](https://github.com/xssbyte/qt_ffmpeg_rtmp_player_demo/blob/main/res/qt_ffmpeg_rtmp_player.gif)  
qt+ffmpeg实现rtmp流播放，播放器代码只有200行。  
功能非常简单，只有播放，停止的demo  
性能：有缓存QImage一次深拷贝，上屏帧率和媒体流帧率一致，也可以使用定时器控制上屏帧率和取帧速率，本地测试延时很低。  
编译：直接build，并把ffmpeg的库拷贝到build文件夹中  
![img](https://github.com/xssbyte/qt_ffmpeg_rtmp_player_demo/blob/main/res/qt_ffmpeg_rtmp_player.png)  
