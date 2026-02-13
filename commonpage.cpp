#include "commonpage.h"
#include "ui_commonpage.h"

#include "listitembox.h"
#include <QDebug>


CommonPage::CommonPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommonPage)
{
    ui->setupUi(this);
    ui->pageMusicList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(ui->playAllBtn, &QPushButton::clicked, this, [=](){
        // 播放所有按钮点击之后，通知QQMusic播放当前pageType标记的page页面中的所有歌曲
        emit playAll(pageType);
    });

    connect(ui->pageMusicList, &QListWidget::doubleClicked, this, [=](const QModelIndex &index){
        // 鼠标双击后，发射信号告诉QQMusic，博能放this页面中共被双击的歌曲
        emit playMusicByIndex(this, index.row());
    });
}

CommonPage::~CommonPage()
{
    delete ui;
}

void CommonPage::setMusicListType(PageType pageType)
{
    this->pageType = pageType;
}

void CommonPage::setCommonPageUI(const QString &text, const QString &imagePath)
{
    ui->PageTittle->setText(text);

    ui->musicImageLabel->setPixmap(QPixmap(imagePath));
    ui->musicImageLabel->setScaledContents(true);
}

void CommonPage::addMusicToMusicPage(MusicList &musicList)
{
    // 1. 清楚之前的歌曲内容
    // 注意：清楚musicOfPage和界面显示无关
    musicOfPage.clear();

    for(auto music : musicList)
    {
        switch(pageType)
        {
        case LIKE_PAGE:
            if(music.getIsLike())
            {
                musicOfPage.push_back(music.getMusicId());
            }
            break;
        case LOCAL_PAGE:
            musicOfPage.push_back(music.getMusicId());
            break;
        case HISTORY_PAGE:
            if(music.getIsHistory())
            {
                 musicOfPage.push_back(music.getMusicId());
            }
            break;
        default:
            qDebug()<<"暂未支持";
        }
    }
}

// 该方法负责将歌曲信息更新到界面
void CommonPage::reFresh(MusicList &musicList)
{
    // 将QListWidget之前已经添加的内容清空
    ui->pageMusicList->clear();

    // 添加新的歌曲
    addMusicToMusicPage(musicList);

    for(auto musicId : musicOfPage)
    {
        auto it = musicList.findMusicByMusicId(musicId);
        if(it == musicList.end())
            continue;

        // 将Music的歌曲名称、作者、专辑名称更新到界面
        // 测试
        // 将ListBoxItem对象放置pageMusicList
        ListItemBox* listItemBox = new ListItemBox(this);
        // 设置歌曲名称、作者、专辑名称更新到界面
        listItemBox->setMusicName(it->getMusicName());
        listItemBox->setMusicSinger(it->getMusicSinger());
        listItemBox->setMusicAlbum(it->getMusicAlbum());
        listItemBox->setLikeMusic(it->getIsLike());

        QListWidgetItem* item = new QListWidgetItem(ui->pageMusicList);
        item->setSizeHint(QSize(listItemBox->width(), listItemBox->height()));
        ui->pageMusicList->setItemWidget(item, listItemBox);

        connect(listItemBox, &ListItemBox::setIsLike, this, [=](bool isLike){
            // 更新歌曲状态
            // 需要通知QQMusic，让其中的likePage、localPage、recentPage更新页面歌曲信息
            emit updataLikeMusic(isLike, it->getMusicId());
        });
    }

    // 触发窗口重绘paintEvent
    // update();   // updata()将paintEvent事件放在事件循环队列中，没有立马处理
    repaint();     // 立马响应paintEvent事件
}

void CommonPage::addMusicToPlaylist(MusicList &musicList, QMediaPlaylist *playList)
{
    for(auto music : musicList)
    {
        switch(pageType)
        {
        case LIKE_PAGE:
            if(music.getIsLike())
            {
                playList->addMedia(music.getMusicUrl());
            }
            break;
        case LOCAL_PAGE:
            playList->addMedia(music.getMusicUrl());
            break;
        case HISTORY_PAGE:
            if(music.getIsHistory())
            {
                playList->addMedia(music.getMusicUrl());
            }
            break;
        default:
            qDebug()<<"未支持页面";
        }
    }
}

void CommonPage::setMusicImage(QPixmap pixmap)
{
    ui->musicImageLabel->setPixmap(pixmap);
    ui->musicImageLabel->setScaledContents(true);
}

QString CommonPage::getMusicIdByIndex(int index)
{
    if(index >= musicOfPage.size())
    {
        qDebug()<<"无此歌曲";
        return "";
    }
    return musicOfPage[index];
}
