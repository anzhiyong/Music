#include "listitembox.h"
#include "ui_listitembox.h"
#include <QDebug>


ListItemBox::ListItemBox(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ListItemBox),
    isLike(false)
{
    ui->setupUi(this);

    connect(ui->likeBtn, &QPushButton::clicked, this, &ListItemBox::onLikeBtnClicked);
}

ListItemBox::~ListItemBox()
{
    qDebug()<<"ListItemBox被销毁";
    delete ui;
}

void ListItemBox::enterEvent(QEvent *event)
{
    (void)event;
    setStyleSheet("background-color:#EFEFEF");
}

void ListItemBox::leaveEvent(QEvent *event)
{
    (void)event;
    setStyleSheet("");
}

void ListItemBox::setMusicName(const QString &musicName)
{
    ui->musicNameLabel->setText(musicName);
}

void ListItemBox::setMusicSinger(const QString &musicSinger)
{
    ui->musicSingerLabel->setText(musicSinger);
}

void ListItemBox::setMusicAlbum(const QString &musicAlbum)
{
    ui->musicAlbumLabel->setText(musicAlbum);
}

void ListItemBox::setLikeMusic(bool isLike)
{
    this->isLike = isLike;
    if(isLike)
    {
        ui->likeBtn->setIcon(QIcon(":/images/like_2.png"));
    }
    else
    {
        ui->likeBtn->setIcon(QIcon(":/images/like_3.png"));
    }
}

void ListItemBox::onLikeBtnClicked()
{
    isLike = !isLike;
    setLikeMusic(isLike);

    // 现在收藏或不收藏的状态改变,应该通知修改Music对象中的isLike属性
    emit setIsLike(isLike);
}



