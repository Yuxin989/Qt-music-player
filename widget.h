#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <unistd.h>
#include <QLabel>
#include <QMap>
#include <QPushButton>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    void changeSongs();
    int val = 0;
    void setVal();
    QLabel *label_lrc_top;
    QLabel *label_lrc_mid;
    QLabel *label_lrc_below;
    QLabel *label_below_back;
    QLabel *label_top_back;
    QLabel *label_mid_back;
    QMap<int,QString> m_lrc;
    QPushButton *skin_1;
    QPushButton *skin_2;
    QPushButton *skin_3;

public slots:
    void play_clicked();
    void mute();
    void last_song();
    void next_song();
    void songs_list();
    void skin_change();
    void lrc_back_change();
    void songs_list_play(QListWidgetItem *current);
    void vol_change();
    void showtime();
    void fast_forward();
    void fast_back();
    void skin_1_change();
    void skin_2_change();
    void skin_3_change();

private:
    Ui::Widget *ui;
    int currentIndex;
    QVector<QString> songList;
    int num = 0;
    QListWidget *listWidget;
    QWidget *widget_Skin;
    QWidget *lrc_back;
    QLabel *label_below;
    QLabel *label_top;
    QLabel *label_mid;

signals:
    void settime();
    void numChange();
};

#endif // WIDGET_H
