# qt_ffmpeg_rtmp_player
qt+ffmpeg实现rtmp流播放，播放器代码只有200多行。  
功能非常简单，只有播放，停止，断线重连，适合新手练手增强自信  
只有缓存QImage一次深拷贝，本地测试延时非常低，效果可以看gif，屏幕窗口几乎是同时动的。  
需要注意的是在qt creator中调试需要把ffmpeg的库拷贝到build出来的文件夹中  
![img](https://github.com/xssbyte/qt_ffmpeg_rtmp_player/blob/main/gif/qt_ffmpeg_rtmp_player.gif)
