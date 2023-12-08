#include "widget.h"
#include "ui_widget.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <QDebug>
#include <dirent.h>
#include <QDir>
#include <QStringList>
#include <random>
#include <QPixmap>
#include <QFile>

int pipe_fd[2];
int time_pos = 0;
bool flag = true;
float timeL = 0.0f;
int l;
float timeS = 0.0f;
int s;

//线程一发送get_percent_pos
void *send_cmd(void *){
    int ret;
    int fd = open("fifo_cmd",O_RDWR);
    if (fd < 0) {
        qDebug()<<"error"<<endl;
        return NULL;
    }
    while (1) {
        usleep(333333);
        if(flag){
            ret = write(fd,"get_percent_pos\n",strlen("get_percent_pos\n"));
            if(ret == -1){
                qDebug() << "get_percent_pos error\n" << endl;
            }
        }
        usleep(333333);
        if(flag){
            ret = write(fd,"get_time_length\n",strlen("get_time_length\n"));
            if(ret == -1){
                qDebug() << "get_time_length error\n" << endl;
            }
        }
        usleep(333333);
        if(flag){
            ret = write(fd,"get_time_pos\n",strlen("get_time_pos\n"));
            if(ret == -1){
                qDebug() << "get_time_pos error\n" << endl;
            }
        }
    }
    ::close(fd);
}

//线程二得到返回的进度百分比
void *recv_msg(void *arg){
    int ret;
    Widget *p = (Widget *)arg;
    while (1) {
        char buf[128] = "";
        if(flag){
            ret = read(pipe_fd[0],buf,sizeof(buf));
            if(ret == -1){
                qDebug() << "read error\n" << endl;
            }
        }
        if(strncmp(buf,"ANS_PERCENT_POSITION",strlen("ANS_PERCENT_POSITION")) == 0){
            time_pos = 0;
            sscanf(buf,"ANS_PERCENT_POSITION=%d",&time_pos);
        }
        if(strncmp(buf,"ANS_LENGTH",strlen("ANS_LENGTH")) == 0){
            timeL = 0;
            sscanf(buf,"ANS_LENGTH=%f",&timeL);
            l = (int)timeL;
        }
        if(strncmp(buf,"ANS_TIME_POSITION",strlen("ANS_TIME_POSITION")) == 0){
            timeS = 0;
            sscanf(buf,"ANS_TIME_POSITION=%f",&timeS);
            s = (int)timeS;
        }
        emit p->settime();
    }
}

//设置歌曲信息 图片 歌词改变
void Widget::changeSongs(){
    QString str = songList.at(num);
    str.remove(0,8).chop(4);

    QStringList parts = str.split('-');
    ui->label_song_name->setText(parts.at(0));
    ui->label_singer_name->setText(parts.at(1));
    ui->label_song_name->setStyleSheet("font-size: 15px; font-weight: bold;");
    ui->label_singer_name->setStyleSheet("font-size: 15px; font-weight: bold;");

    QList<QListWidgetItem *> items = listWidget->findItems(str, Qt::MatchContains);
    if (!items.isEmpty()) {
        // 获取第一个匹配的项并居中显示
        QListWidgetItem *item = items.first();
        listWidget->setCurrentItem(item);
        listWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }

    char cmd[128];
    sprintf(cmd,"./image/%s.png",qPrintable(str));
    QPixmap pixmap(cmd);
    ui->label_pic->setPixmap(pixmap);
    ui->label_pic->setScaledContents(true);

    int minute,second;
    int lrc_time;
    sprintf(cmd,"./lrc/%s.lrc",qPrintable(str));
    m_lrc.clear();
    QFile file(cmd);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            parts = line.split(']');
            sscanf(parts.at(0).toUtf8().constData(),"[%d:%d:000",&minute,&second);
            lrc_time = minute * 60 + second;
            m_lrc.insert(lrc_time,parts.at(1));
        }
        file.close();
    }
}

//发送num改变的信号
void Widget::setVal()
{
    if(val != num){
        val = num;
        emit numChange();
    }
}

//线程三执行发num改变信号的函数
void *num_signal(void *arg){
    Widget *p = (Widget *)arg;
    while (1) {
        if(flag){
            p->setVal();
            usleep(100000);
        }
    }
}

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    currentIndex(0)
{
    //歌曲列表 初始设置隐藏
    listWidget = new QListWidget(this);
    listWidget->setHidden(true);
    listWidget->setGeometry(800,240,200,300);

    //壁纸块 初始为灰色并置于底层
    label_top = new QLabel(this);
    label_top->setGeometry(0,0,1000,30);
    label_top->show();
    label_top->lower();
    label_top->setStyleSheet("background-color: lightGray; color: white;");

    label_mid = new QLabel(this);
    label_mid->setGeometry(0,30,1000,470);
    label_mid->show();
    label_mid->lower();
    label_mid->setStyleSheet("background-color: lightGray; color: white;");

    label_below = new QLabel(this);
    label_below->setGeometry(0,500,1000,100);
    label_below->show();
    label_below->lower();
    label_below->setStyleSheet("background-color: lightGray; color: white;");

    //主界面歌词块 中间是正在播放的歌词
    label_lrc_top = new QLabel(this);
    label_lrc_top->setGeometry(410,50,500,180);
    label_lrc_top->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    label_lrc_top->setStyleSheet("font-size: 22px; line-height: 60px;");
    label_lrc_top->show();
    label_lrc_top->raise();

    label_lrc_mid = new QLabel(this);
    label_lrc_mid->setGeometry(410,230,500,40);
    label_lrc_mid->setAlignment(Qt::AlignHCenter);
    label_lrc_mid->setStyleSheet("color: red; font-size: 22px; line-height: 60px;");
    label_lrc_mid->show();
    label_lrc_mid->raise();

    label_lrc_below = new QLabel(this);
    label_lrc_below->setGeometry(410,265,500,180);
    label_lrc_below->setAlignment(Qt::AlignHCenter);
    label_lrc_below->setStyleSheet("font-size: 22px; line-height: 60px;");
    label_lrc_below->show();
    label_lrc_below->raise();

    //换皮肤的三个按钮所在的界面  初始设置为隐藏
    widget_Skin = new QWidget(this);
    widget_Skin->setGeometry(870,30,130,50);
    widget_Skin->setStyleSheet("background-color: lightGray;");
    widget_Skin->setHidden(true);

    //纯歌词界面 初始设置为隐藏
    lrc_back = new QWidget(this);
    lrc_back->setGeometry(0,30,1000,570);
    lrc_back->setStyleSheet("background-color: lightGray;");
    lrc_back->setHidden(true);

    //纯歌词界面的三个歌词框
    label_top_back = new QLabel(lrc_back);
    label_top_back->setGeometry(90,50,820,180);
    label_top_back->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    label_top_back->setStyleSheet("font-size: 22px; line-height: 60px;");
    label_top_back->show();
    label_top_back->raise();

    label_mid_back = new QLabel(lrc_back);
    label_mid_back->setGeometry(90,230,820,40);
    label_mid_back->setAlignment(Qt::AlignHCenter);
    label_mid_back->setStyleSheet("color: red; font-size: 22px; line-height: 60px;");
    label_mid_back->show();
    label_mid_back->raise();

    label_below_back = new QLabel(lrc_back);
    label_below_back->setGeometry(90,265,820,180);
    label_below_back->setAlignment(Qt::AlignHCenter);
    label_below_back->setStyleSheet("font-size: 22px; line-height: 60px;");
    label_below_back->show();
    label_below_back->raise();

    //换肤按钮1 2 3
    skin_1 = new QPushButton("蓝粉",widget_Skin);
    skin_1->setGeometry(10,10,30,30);

    skin_2 = new QPushButton("冰心",widget_Skin);
    skin_2->setGeometry(50,10,30,30);

    skin_3 = new QPushButton("纯",widget_Skin);
    skin_3->setGeometry(90,10,30,30);

    //处理歌曲文件名
    QDir dir("./songs");
    QStringList fileNames = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    // 将文件名存放到数组中
    foreach (QString fileName, fileNames) {
        songList.append("./songs/" + fileName);
        QString trimmedFileName = fileName.left(fileName.length() - 4);
        QListWidgetItem *item = new QListWidgetItem(trimmedFileName);
        item->setFlags(item->flags() | Qt::ItemIsSelectable);
        listWidget->addItem(item);
    }
    //设置随机数 执行程序随机播放列表中的一首歌曲
    srand(time(NULL));
    num = rand() % songList.size();

    //创建管道及父子进程
    mkfifo("fifo_cmd",0777);
    int ret = pipe(pipe_fd);
    if(ret == -1){
        qDebug() << "pipe error\n" << endl;
    }
    pid_t pid = fork();
    if(pid <0){
        exit(-1);
    }else if(pid == 0){
        //子进程执行mplayer
        dup2(pipe_fd[1],1);
        execlp("mplayer","mplayer","-slave","-quiet","-idle","-input","file=./fifo_cmd","-volume","60",qPrintable(songList.at(num)),NULL);
    }else if(pid >0){
        ui->setupUi(this);

        //放置歌曲信息以及专辑图片的label
        ui->label_song_name->setAlignment(Qt::AlignCenter);
        ui->label_singer_name->setAlignment(Qt::AlignCenter);
        ui->label_pic->setAlignment(Qt::AlignCenter);
        ui->label_pic->setStyleSheet("border-radius: 50%;");
        //播放和暂停按钮
        connect(ui->play,&QPushButton::clicked,this,&Widget::play_clicked);
        connect(ui->mute,&QPushButton::clicked,this,&Widget::mute);
        //上一曲下一曲
        connect(ui->last,&QPushButton::clicked,this,&Widget::last_song);
        connect(ui->next,&QPushButton::clicked,this,&Widget::next_song);
        //歌曲列表显示以及点击列表切歌
        connect(ui->list,&QPushButton::clicked,this,&Widget::songs_list);
        connect(listWidget,&QListWidget::currentItemChanged,this,&Widget::songs_list_play);
        //换肤按钮以及纯歌词模式按钮
        connect(ui->skin,&QPushButton::clicked,this,&Widget::skin_change);
        connect(ui->lrc,&QPushButton::clicked,this,&Widget::lrc_back_change);
        //音量进度条设置
        ui->volume->setRange(0,100);
        ui->volume->setValue(60);
        ui->volume->setSingleStep(1);
        connect(ui->volume,&QSlider::valueChanged,this,&Widget::vol_change);

        connect(this,&Widget::settime,this,&Widget::showtime);
        connect(this,&Widget::numChange,this,&Widget::changeSongs);

        pthread_t tid1;
        pthread_create(&tid1,NULL,send_cmd,NULL);
        pthread_detach(tid1);

        pthread_t tid2;
        pthread_create(&tid2,NULL,recv_msg,(void *)this);
        pthread_detach(tid2);

        pthread_t tid3;
        pthread_create(&tid3,NULL,num_signal,(void *)this);
        pthread_detach(tid3);
        //快进和快退按钮 默认设置跨度五秒
        connect(ui->fast,&QPushButton::clicked,this,&Widget::fast_forward);
        connect(ui->rewind,&QPushButton::clicked,this,&Widget::fast_back);
        //三个皮肤
        connect(skin_1,&QPushButton::clicked,this,&Widget::skin_1_change);
        connect(skin_2,&QPushButton::clicked,this,&Widget::skin_2_change);
        connect(skin_3,&QPushButton::clicked,this,&Widget::skin_3_change);

        //设置按钮鼠标悬停文字
        ui->play->setToolTip("播放/暂停");
        ui->last->setToolTip("上一曲");
        ui->next->setToolTip("下一曲");
        ui->fast->setToolTip("快进");
        ui->rewind->setToolTip("快退");
        ui->mute->setToolTip("静音/取消静音");
        ui->list->setToolTip("歌曲列表");
        ui->skin->setToolTip("换肤");
        ui->lrc->setToolTip("纯歌词模式");
        skin_1->setToolTip("蓝粉岁月");
        skin_2->setToolTip("冰心");
        skin_3->setToolTip("纯图片背景");
        ui->volume->setToolTip(QString::number(ui->volume->value()));
    }
}

//暫停播放按鈕
void Widget::play_clicked()
{
    int fd = open("fifo_cmd",O_RDWR);
    int ret = write(fd,"pause\n",strlen("pause\n"));
    if(ret == -1){
        qDebug() << "pause error\n" << endl;
    }
    ::close(fd);
    flag = !flag;
}

//靜音按鈕
void Widget::mute()
{
    int fd = open("fifo_cmd",O_RDWR);
    int ret = write(fd,"m\n",strlen("m\n"));
    if(ret == -1){
        qDebug() << "mute error\n" << endl;
    }
    ::close(fd);
    if(ui->volume->value() == 0){
        ui->volume->setValue(60);
    }else{
        ui->volume->setValue(0);
    }
}

//上一曲
void Widget::last_song()
{
    //到列表的第一曲再次点击播放最后一曲
    char cmd[128];
    if(num > 0){
        num--;
        int fd = open("fifo_cmd",O_RDWR);
        sprintf(cmd,"loadfile %s\n",qPrintable(songList.at(num)));
        int ret = write(fd,cmd,strlen(cmd));
        if(ret == -1){
            qDebug() << "last_song error\n" << endl;
        }
        ::close(fd);
    }else if(num == 0){
        num = songList.size()-1;
        int fd = open("fifo_cmd",O_RDWR);
        sprintf(cmd,"loadfile %s\n",qPrintable(songList.at(num)));
        int ret = write(fd,cmd,strlen(cmd));
        if(ret == -1){
            qDebug() << "last_song error\n" << endl;
        }
        ::close(fd);
    }
}

//下一曲
void Widget::next_song()
{
    //到列表的最后一曲 再次点击播放第一曲
    char cmd[128];
    if(num < songList.size()-1){
        num++;
        int fd = open("fifo_cmd",O_RDWR);
        sprintf(cmd,"loadfile %s\n",qPrintable(songList.at(num)));
        int ret = write(fd,cmd,strlen(cmd));
        if(ret == -1){
            qDebug() << "next_song error\n" << endl;
        }
        ::close(fd);
    }else if(num == songList.size()-1){
        num = 0;
        int fd = open("fifo_cmd",O_RDWR);
        sprintf(cmd,"loadfile %s\n",qPrintable(songList.at(num)));
        int ret = write(fd,cmd,strlen(cmd));
        if(ret == -1){
            qDebug() << "next_song error\n" << endl;
        }
        ::close(fd);
    }
}

//显示歌曲列表
void Widget::songs_list()
{
    if (listWidget->isVisible()) {
        listWidget->hide();
    } else {
        listWidget->show();
        listWidget->raise();
    }
}

//更改皮肤
void Widget::skin_change()
{
    if (widget_Skin->isVisible()){
        widget_Skin->hide();
    } else {
        widget_Skin->show();
        widget_Skin->raise();
    }
}

//纯歌词页面
void Widget::lrc_back_change()
{
    if (lrc_back->isVisible()){
        lrc_back->hide();
    } else {
        lrc_back->show();
        lrc_back->raise();
    }
}

//歌曲列表切歌
void Widget::songs_list_play(QListWidgetItem *current)
{
    //保证列表切歌之后再下一曲是点击的下一曲
    char temp[128];
    sprintf(temp,"./songs/%s.mp3",qPrintable(current->text()));
    for(int i = 0;i < songList.size();i++){
        if(songList.at(i) == temp){
            num = i;
            break;
        }
    }
    int fd = open("fifo_cmd",O_RDWR);
    char cmd[128];
    sprintf(cmd,"loadfile ./songs/%s.mp3\n",qPrintable(current->text()));
    int ret = write(fd,cmd,strlen(cmd));
    if(ret == -1){
        qDebug() << "songs_list_play error\n" << endl;
    }
    ::close(fd);
}

//声音进度条控制
void Widget::vol_change()
{
    int fd = open("fifo_cmd",O_RDWR);
    char cmd[128];
    sprintf(cmd,"volume %d 1\n",ui->volume->value());
    int ret = write(fd,cmd,strlen(cmd));
    if(ret == -1){
        qDebug() << "vol_change error\n" << endl;
    }
    ::close(fd);
    ui->volume->setToolTip(QString::number(ui->volume->value()));
}

//播放进度条设置 以及歌词滚动
void Widget::showtime()
{
    ui->progress->setRange(0,100);
    ui->progress->setValue(time_pos);
    char cmdL[8],cmdS[8];
    sprintf(cmdS,"%d:%.2d",s/60,s%60);
    sprintf(cmdL,"%d:%.2d",l/60,l%60);
    ui->label_pos->setText(cmdS);
    ui->label_len->setText(cmdL);

    //播放进度大于等于99时 自动切换下一曲
    //经过多次测试 mplayer返回的进度大多数情况下无法走到100 所以判断条件如果是=100的话 大多数情况不会自动切歌
    if(time_pos >= 99){
        char cmd[128];
        if(num < songList.size()-1){
            num++;
            int fd = open("fifo_cmd",O_RDWR);
            sprintf(cmd,"loadfile %s\n",qPrintable(songList.at(num)));
            int ret = write(fd,cmd,strlen(cmd));
            if(ret == -1){
                qDebug() << "next_song error\n" << endl;
            }
            ::close(fd);
        }else if(num == songList.size()-1){
            num = 0;
            int fd = open("fifo_cmd",O_RDWR);
            sprintf(cmd,"loadfile %s\n",qPrintable(songList.at(num)));
            int ret = write(fd,cmd,strlen(cmd));
            if(ret == -1){
                qDebug() << "next_song error\n" << endl;
            }
            ::close(fd);
        }
        time_pos = 0;
    }

    //设置歌词显示的三个label框
    QMap<int, QString>::const_iterator it = m_lrc.constBegin();
    for (; it != m_lrc.constEnd(); it++) {
        if (it.key() == s){
            label_lrc_mid->setText(it.value());
            label_mid_back->setText(it.value());

            QMap<int, QString>::const_iterator midIt = it;
            QMap<int, QString>::const_iterator topIt = midIt;

            // 在 label_lrc_mid 周围定位后，将 midIt 移动到 label_lrc_mid 后一句
            if (midIt != m_lrc.constEnd()) {
                midIt++; // 移动到 label_lrc_mid 后一句
            }

            // 显示 label_lrc_top
            QString topText;
            int topCount = 0;
            while (topCount < 5 && topIt != m_lrc.constBegin()) {
                topIt--;
                topCount++;
            }
            for (; topCount > 0 && topIt != m_lrc.constEnd(); topIt++, topCount--) {
                topText += topIt.value() + '\n';
            }
            label_lrc_top->setText(topText.trimmed());
            label_top_back->setText(topText.trimmed());

            // 显示 label_lrc_below
            if (it != m_lrc.constEnd()) {
                QMap<int, QString>::const_iterator nextIt = it;
                nextIt++; // 移动到 label_lrc_mid 后一句

                QString belowText;
                int belowCount = 0;
                // 显示 label_lrc_below
                while (belowCount < 5 && nextIt != m_lrc.constEnd()) {
                    belowText += nextIt.value() + '\n';
                    nextIt++;
                    belowCount++;
                }
                label_lrc_below->setText(belowText.trimmed());
                label_below_back->setText(belowText.trimmed());
            }
        }
    }
}

//快进5秒
void Widget::fast_forward()
{
    int fd = open("fifo_cmd",O_RDWR);
    int ret = write(fd,"seek 5\n",strlen("seek 5\n"));
    if(ret == -1){
        qDebug() << "fast_forward error\n" << endl;
    }
    ::close(fd);
}

//快退5秒
void Widget::fast_back()
{
    int fd = open("fifo_cmd",O_RDWR);
    int ret = write(fd,"seek -5\n",strlen("seek -5\n"));
    if(ret == -1){
        qDebug() << "fast_back error\n" << endl;
    }
    ::close(fd);
}

//切换皮肤123
void Widget::skin_1_change()
{
    label_top->clear();
    label_top->setStyleSheet("background-color: cyan; color: white;");
    QPixmap pixmap_mid("./image/background.png");
    label_mid->setPixmap(pixmap_mid);
    label_below->clear();
    label_below->setStyleSheet("background-color: cyan; color: white;");
}

void Widget::skin_2_change()
{
    label_top->clear();
    label_top->setStyleSheet("background-color: blue; color: white;");
    QPixmap pixmap_mid("./image/background_2.png");
    label_mid->setPixmap(pixmap_mid);
    label_below->clear();
    label_below->setStyleSheet("background-color: blue; color: white;");
}

void Widget::skin_3_change()
{
    QPixmap pixmap_top("./image/background_top.png");
    label_top->setPixmap(pixmap_top);
    QPixmap pixmap_mid("./image/background_mid.png");
    label_mid->setPixmap(pixmap_mid);
    QPixmap pixmap_below("./image/background_below.png");
    label_below->setPixmap(pixmap_below);
}

Widget::~Widget()
{
    delete listWidget;
    delete ui;
}
