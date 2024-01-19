# 小型音乐播放器

#### 介绍
​	一个基于linux系统的小型音乐播放器，大佬勿喷，这个东西只能读取放在文件里的音乐，只是一个初学者练手小qt项目。

#### 软件架构
没有什么架构，主要用qt完成，代码全部是C++

#### 功能简述

1. 在执行的同时打开mplayer播放器播放音乐
2. 实现上一曲、下一曲的切换
3. 中间部分显示歌曲信息、专辑图片并随着歌曲切换而切换
4. 实现歌词滚动效果
5. 实现换肤功能以及纯歌词模式显示
6. 鼠标悬停在按钮以及音量进度条会显示提示


#### 编译运行

```c++
qmake
make
./yx_music
```

注：如果没有mplayer播放器，提前安装

```
sudo apt update
sudo apt install mplayer
```

#### 参与贡献

完全由本人一人编写完成，初学者，翔山代码，不喜勿喷
