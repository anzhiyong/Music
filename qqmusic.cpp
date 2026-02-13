#include "qqmusic.h"
#include "ui_qqmusic.h"

#include <QMouseEvent>
#include <QDebug>

#include <QGraphicsDropShadowEffect>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSystemTrayIcon>
#include <QMenu>



QQMusic::QQMusic(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::QQMusic)
    , currentIndex(-1)
{
    ui->setupUi(this);

    initUi();

    initSqlite();

    initPlayer();

    initMusicList();

    connectSignalAndSlots();
}

QQMusic::~QQMusic()
{
    delete ui;
}

void QQMusic::initUi()
{
    this->setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon(":/images/tubiao.png"));
    ui->max->setEnabled(false);

    // 添加系统托盘
    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/images/tubiao.png"));
    trayIcon->show();
    // 托盘添加菜单
    QMenu* trayMenu = new QMenu();
    trayMenu->addAction("显示", this, &QWidget::showNormal);
    trayMenu->addAction("退出", this, &QQMusic::onQQMusicQuit);
    trayIcon->setContextMenu(trayMenu);

    // 窗口添加阴影效果
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);

    shadowEffect->setOffset(0, 0);
    shadowEffect->setColor("#000000");   // 黑色
    shadowEffect->setBlurRadius(10);
    this->setGraphicsEffect(shadowEffect);

    // 设置BtForm图标 & 文本信息
    ui->rec->setIconAndText(":/images/rec.png", "推荐", 0);
    ui->radio->setIconAndText(":/images/radio.png", "电台", 1);
    ui->music->setIconAndText(":/images/music.png", "音乐馆", 2);
    ui->like->setIconAndText(":/images/like.png", "我喜欢", 3);
    ui->local->setIconAndText(":/images/local.png", "本地下载", 4);
    ui->recent->setIconAndText(":/images/recent.png", "最近播放", 5);

    // 将localPage设置为当前页面
    ui->stackedWidget->setCurrentIndex(4);
    currentPage = ui->localPage;
    // 本地下载BtForm动画默认显示
    ui->local->showAnimal(true);

    // 初始化推荐页面
    srand(time(NULL));
    ui->recMusicBox->initRecBoxUi(randomPiction(), 1);
    ui->supplyMuscBox->initRecBoxUi(randomPiction(), 2);

    // 设置CommonPage的信息
    ui->likePage->setCommonPageUI("我喜欢", ":/images/ilikebg.png");
    ui->localPage->setCommonPageUI("本地音乐", ":/images/localbg.png");
    ui->recentPage->setCommonPageUI("最近播放", ":/images/recentbg.png");

    // 播放控制区按钮图标设定
    ui->play->setIcon(QIcon(":/images/play_2.png"));
    ui->playMode->setIcon(QIcon(":/images/shuffle_2.png"));


    volumeTool = new VolumeTool(this);

    // 实例化LrcWord对象
    lrcPage = new LrcPage(this);
    lrcPage->setGeometry(10, 10, lrcPage->width(), lrcPage->height());
    lrcPage->hide();

    // 初始化上移动画对象
    lrcPageAnimal = new QPropertyAnimation(lrcPage, "geometry", this);
    lrcPageAnimal->setDuration(500);
    lrcPageAnimal->setStartValue(QRect(10, 10+lrcPage->height(), lrcPage->width(), lrcPage->height()));
    lrcPageAnimal->setEndValue(QRect(10, 10, lrcPage->width(), lrcPage->height()));
}

void QQMusic::initSqlite()
{
    // 1. 进行数据库驱动加载
    QSqlDatabase sqlite = QSqlDatabase::addDatabase("QSQLITE");

    // 2. 设置数据库名称
    sqlite.setDatabaseName("QQMusic.db");

    // 3. 打开
    if(!sqlite.open())
    {
        QMessageBox::critical(this, "QQMusic", "数据库打开失败!!!");
        return;
    }

    qDebug()<<"QQMusic数据库连接成功";

    // 4. 创建表
    QString sql = "CREATE TABLE IF NOT EXISTS MusicInfo(\
                      id INTEGER PRIMARY KEY AUTOINCREMENT,\
                      musicId varchar(50) UNIQUE,\
                      musicName varchar(50),\
                      musicSinger varchar(50), \
                      albumName varchar(50),\
                      musicUrl varchar(256),\
                      duration BIGINT,\
                      isLike INTEGER,\
                      isHistory INTEGER)";

    QSqlQuery query;
    if(!query.exec(sql))
    {
        QMessageBox::critical(this, "QQMusic", "初始化错误!!!");
        return;
    }

    qDebug()<<"QQMusic表创建成功!!!";
}

void QQMusic::initMusicList()
{
    musicList.readFromDB();

    ui->likePage->setMusicListType(PageType::LIKE_PAGE);
    ui->likePage->reFresh(musicList);

    ui->localPage->setMusicListType(PageType::LOCAL_PAGE);
    ui->localPage->reFresh(musicList);
    // 将localPage中的歌曲添加到媒体播放列表中
    ui->localPage->addMusicToPlaylist(musicList, playList);

    ui->recentPage->setMusicListType(PageType::HISTORY_PAGE);
    ui->recentPage->reFresh(musicList);
}

void QQMusic::initPlayer()
{
    // 1. 初始化媒体播相关类对象
    player = new QMediaPlayer(this);
    playList = new QMediaPlaylist(this);

    // 2. 设置默认播放模式
    playList->setPlaybackMode(QMediaPlaylist::Random);

    // 3. 将播放列表设置到播放媒体对象中
    player->setPlaylist(playList);

    // 4. 设置默认的音量大小, 默认设置成20%
    player->setVolume(20);


    // 关联QMediaPlayer的信号
    connect(player, &QMediaPlayer::stateChanged, this, &QQMusic::onPlayerStateChanged);

    // 关联QMediaPlayer::durationChanged信号
    connect(player, &QMediaPlayer::durationChanged, this, &QQMusic::onDurationChanged);

    // 关联QMediaPlayer::positionChanged信号
    connect(player, &QMediaPlayer::positionChanged, this, &QQMusic::onPositionChanged);

    // 关联QMediaPlayer::metaDataAvailableChanged
    connect(player, &QMediaPlayer::metaDataAvailableChanged, this, &QQMusic::onMetaDataAvailableChanged);

    // 播放列表的模式放生改变时的信号槽关联
    connect(playList, &QMediaPlaylist::playbackModeChanged, this, &QQMusic::onPlaybackModeChanged);

    // 当playlist中播放源发生变化时
    connect(playList, &QMediaPlaylist::currentIndexChanged, this, &QQMusic::onCurrentIndexChanged);
}

QJsonArray QQMusic::randomPiction()
{
    // 推荐文本 + 推荐图片路径
    QVector<QString> vecImageName;
    vecImageName<<"001.png"<<"003.png"<<"004.png"<<"005.png"<<"006.png"<<"007.png"
                <<"008.png"<<"009.png"<<"010.png"<<"011.png"<<"012.png"<<"013.png"
                <<"014.png"<<"015.png"<<"016.png"<<"017.png"<<"018.png"<<"019.png"
                <<"020.png"<<"021.png"<<"022.png"<<"023.png"<<"024.png"<<"025.png"
                <<"026.png"<<"027.png"<<"028.png"<<"029.png"<<"030.png"<<"031.png"
                <<"032.png"<<"033.png"<<"034.png"<<"035.png"<<"036.png"<<"037.png"
                <<"038.png"<<"039.png"<<"040.png";

    std::random_shuffle(vecImageName.begin(), vecImageName.end());

    // 001.png
    // path: ":/images/rec/"+vecImageName[i];
    // text: "推荐-001"
    QJsonArray objArray;
    for(int i = 0; i < vecImageName.size(); ++i)
    {
        QJsonObject obj;
        obj.insert("path", ":/images/rec/"+vecImageName[i]);

        QString strText = QString("推荐-%1").arg(i, 3, 10, QChar('0'));
        obj.insert("text", strText);

        objArray.append(obj);

    }

    return objArray;
}

void QQMusic::connectSignalAndSlots()
{
    // 关联BtForm的信号和处理该信号的槽函数
    connect(ui->rec, &BtForm::btClick, this, &QQMusic::onBtFormClick);
    connect(ui->radio, &BtForm::btClick, this, &QQMusic::onBtFormClick);
    connect(ui->music, &BtForm::btClick, this, &QQMusic::onBtFormClick);
    connect(ui->like, &BtForm::btClick, this, &QQMusic::onBtFormClick);
    connect(ui->local, &BtForm::btClick, this, &QQMusic::onBtFormClick);
    connect(ui->recent, &BtForm::btClick, this, &QQMusic::onBtFormClick);

    // 收藏或者不收藏处理
    connect(ui->likePage, &CommonPage::updataLikeMusic, this, &QQMusic::updateLikeMusicAndPage);
    connect(ui->localPage, &CommonPage::updataLikeMusic, this, &QQMusic::updateLikeMusicAndPage);
    connect(ui->recentPage, &CommonPage::updataLikeMusic, this, &QQMusic::updateLikeMusicAndPage);


    // 播放控制区的信号和槽函数关联
    connect(ui->play, &QPushButton::clicked, this, &QQMusic::onPlayMusic);
    connect(ui->playUp, &QPushButton::clicked, this, &QQMusic::onPlayUpClicked);
    connect(ui->playDown, &QPushButton::clicked, this, &QQMusic::onPlayDownClicked);
    connect(ui->playMode, &QPushButton::clicked, this, &QQMusic::onPlaybackModeClicked);

    // likePage、localPage、recentPage都有播放所有按钮
    connect(ui->likePage, &CommonPage::playAll, this, &QQMusic::onPlayAll);
    connect(ui->localPage, &CommonPage::playAll, this, &QQMusic::onPlayAll);
    connect(ui->recentPage, &CommonPage::playAll, this, &QQMusic::onPlayAll);

    // 处理likePage、localPage、recentPage中ListItemBox双击
    connect(ui->likePage, &CommonPage::playMusicByIndex, this, &QQMusic::playMusicByIndex);
    connect(ui->localPage, &CommonPage::playMusicByIndex, this, &QQMusic::playMusicByIndex);
    connect(ui->recentPage, &CommonPage::playMusicByIndex, this, &QQMusic::playMusicByIndex);

    // 歌词按钮点击信号和槽函数
    connect(ui->lrcWord, &QPushButton::clicked, this, &QQMusic::onLrcWordClicked);

    // 处理静音
    connect(volumeTool, &VolumeTool::setMusicMuted, this, &QQMusic::setPlayerMuted);

    // 处理音量大小
    connect(volumeTool, &VolumeTool::setMusicVolume, this, &QQMusic::setPlayerVolume);

    // MusicSlider::setMusicSliderPosiont
    connect(ui->progressBar, &MusicSlider::setMusicSliderPositon, this, &QQMusic::onMusicSliderChanged);
}

void QQMusic::updateBtformAnimal()
{
    // 获取currentPage在stackedWidget中的索引
    int index = ui->stackedWidget->indexOf(currentPage);
    if(-1 == index)
    {
        qDebug()<<"该页面不存在";
        return;
    }

    // 获取QQMusci界面上所有的btForm
    QList<BtForm*> btForms = this->findChildren<BtForm*>();
    for(auto btForm : btForms)
    {
        if(btForm->getPageId() == index)
        {
            // 将currentPage对鱼竿的btForm找到了
            btForm->showAnimal(true);
        }
        else
        {
            btForm->showAnimal(false);
        }
    }
}

void QQMusic::onQQMusicQuit()
{
    // 歌曲信息写入数据库
    musicList.writeToDB();

    // 断开与SQLite的链接
    sqlite.close();

    // 关闭窗口
    close();
}

void QQMusic::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() == Qt::LeftButton && isDrag)
    {
        move(event->globalPos() - dragPos);

        qDebug()<<"mouse move";
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void QQMusic::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        isDrag = true;
        // 获取鼠标相对于电脑屏幕左上角的全局坐标
        dragPos = event->globalPos() - geometry().topLeft();
        return;
    }

     QWidget::mousePressEvent(event);
}

void QQMusic::on_quit_clicked()
{
    hide();
}

void QQMusic::onBtFormClick(int pageId)
{
    // 清除之前btForm按钮的颜色背景
    // 获取所有的BtForm按钮
    QList<BtForm*> btFormList = this->findChildren<BtForm*>();
    for(auto btForm : btFormList)
    {
        if(btForm->getPageId() != pageId)
        {
            btForm->clearBackground();
        }
    }

    ui->stackedWidget->setCurrentIndex(pageId);
    qDebug()<<"切换页面"<<pageId;

    isDrag = false;
}

void QQMusic::updateLikeMusicAndPage(bool isLike, const QString &musicId)
{
    // 1. 修改状态
    auto it = musicList.findMusicByMusicId(musicId);
    if(it != musicList.end())
    {
        it->setIsLike(isLike);
    }

    // 2. 更新Page页面的歌曲列表
    ui->likePage->reFresh(musicList);
    ui->localPage->reFresh(musicList);
    ui->recentPage->reFresh(musicList);
}

void QQMusic::on_volume_clicked()
{
    // 1. 获取ui->volume控件的left-top坐标，并转换为基于屏幕的全局坐标
    QPoint point = ui->volume->mapToGlobal(QPoint(0, 0));

    // 2. 计算volumeTool需要移动到的位置：即ui->volume的正上方偏左一半
    QPoint volumeLeftTop = point - QPoint(volumeTool->width()/2, volumeTool->height());

    volumeLeftTop.setX(volumeLeftTop.x()+15);
    volumeLeftTop.setY(volumeLeftTop.y()+30);

    // 3. 移动volumeTool
    volumeTool->move(volumeLeftTop);

    // 4. 窗口显示
    volumeTool->show();
}

void QQMusic::on_addLocal_clicked()
{
    QFileDialog fileDialog(this);

    // 设置窗口的标题
    fileDialog.setWindowTitle("添加本地下载音乐");

    // 设置文件对话框默认的打开路径
    QDir dir(QDir::currentPath());
    dir.cdUp();
    QString projectPaht = dir.path();
    projectPaht += "/QQMusic/musics/";
    qDebug()<<projectPaht;
    fileDialog.setDirectory(projectPaht);


    // 设置一次多开多个
    fileDialog.setFileMode(QFileDialog::ExistingFiles);

    // 通过文件后缀
    // fileDialog.setNameFilter("代码文件(*.h *.c *.hpp *.cpp)");
    // fileDialog.setNameFilter("图片文件(*.png *.bmp *.jpg)");

    // 通过文件的MIME类型来过滤
    QStringList mimeTypeFilters;
     mimeTypeFilters << "application/octet-stream";    // All(*)
     fileDialog.setMimeTypeFilters(mimeTypeFilters);

    if(QDialog::Accepted == fileDialog.exec())
    {
        // 获取选中的文件
        QList<QUrl> fileUrls = fileDialog.selectedUrls();

        // fileUrls: 内部存放的是刚刚选中的文件的url路径
        // 需要将文件信息填充到本地下载

        // 将所有音乐添加到音乐列表中，MusicList
        // musicList中管理的是解析之后的Music对象
        musicList.addMusicsByUrl(fileUrls);

        // 将歌曲信息更新到CommonPage页面的的listWidget中
        // 将CommonPage切换到本地下载的页面
        ui->stackedWidget->setCurrentIndex(4);

        ui->localPage->reFresh(musicList);

        // 将localPage中的歌曲添加到媒体播放列表中
        ui->localPage->addMusicToPlaylist(musicList, playList);
    }
}

void QQMusic::onLrcWordClicked()
{
    lrcPage->show();
    lrcPageAnimal->start();
}


///////////////////////////////////////////////////////////////////////
// 播放控制说明

// 播放和暂停
void QQMusic::onPlayMusic()
{
    if(QMediaPlayer::PlayingState == player->state())
    {
        // 播放中点击，应该暂停
        player->pause();
        //ui->play->setIcon(QIcon(":/images/play3.png"));
    }
    else if(QMediaPlayer::PausedState == player->state())
    {
        // 暂停状态中点击按钮，应该继续播放
        player->play();
        //ui->play->setIcon(QIcon(":/images/play_on.png"));
    }
    else if(QMediaPlayer::StoppedState == player->state())
    {
        // 刚开始默认为停止状态，当点击播放按钮时，直接播放即可
        player->play();
        //ui->play->setIcon(QIcon(":/images/play_on.png"));
    }
    else
    {
        qDebug()<<player->errorString();
    }
}

void QQMusic::onPlayUpClicked()
{
    playList->previous();
}

void QQMusic::onPlayDownClicked()
{
    playList->next();
}

void QQMusic::onPlaybackModeClicked()
{
    // 1. 根据当前播放模式，切换到下一个播放模式
    // 2. 设置ToolTip提示
    // 列表循环--->随机播放--->单曲循环
    // 当状态发生改变的时候，还需要修改按钮上的图标
    if(playList->playbackMode() == QMediaPlaylist::Loop)
    {
        // 随机播放
        playList->setPlaybackMode(QMediaPlaylist::Random);
        ui->playMode->setToolTip("随机播放");
    }
    else if(playList->playbackMode() == QMediaPlaylist::Random)
    {
        // 单曲循环
        playList->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
        ui->playMode->setToolTip("单曲循环");
    }
    else if(playList->playbackMode() == QMediaPlaylist::CurrentItemInLoop)
    {
        playList->setPlaybackMode(QMediaPlaylist::Loop);
        ui->playMode->setToolTip("列表循环");
    }
    else
    {
        // 暂不支持
        qDebug()<<"暂不支持";
    }
}

void QQMusic::onPlayAll(PageType pageType)
{
    qDebug()<<"播放所有";
    // 播放page对应的CommonPage页面中的所有歌曲，默认从第0首开始播放
    // 通过pageType找到对应的CommonPage页面
    CommonPage* page = ui->localPage;
    switch(pageType)
    {
    case PageType::LIKE_PAGE:
        page = ui->likePage;
        break;
    case PageType::LOCAL_PAGE:
        page = ui->localPage;
        break;
    case PageType::HISTORY_PAGE:
        page = ui->recentPage;
        break;
    default:
        qDebug()<<"暂不支持的页面";
    }

    // page中记录的就是要播放的页面
    playAllMusicOfCommonPage(page, 0);
}

void QQMusic::playAllMusicOfCommonPage(CommonPage *page, int index)
{
    currentPage = page;
    updateBtformAnimal();

    // 清空之前playlist中的歌曲
    playList->clear();

    // 添加当前要播放的page页面中的所有歌曲
    page->addMusicToPlaylist(musicList, playList);

    // 播放从第0首开始播放
    playList->setCurrentIndex(index);

    player->play();
}

void QQMusic::playMusicByIndex(CommonPage *page, int index)
{
    playAllMusicOfCommonPage(page, index);
}

void QQMusic::setPlayerMuted(bool isMuted)
{
    player->setMuted(isMuted);
}

void QQMusic::setPlayerVolume(int volume)
{
    player->setVolume(volume);
}

void QQMusic::onPlayerStateChanged()
{
    if(player->state() == QMediaPlayer::PlayingState)
    {
        ui->play->setIcon(QIcon(":/images/play_on.png"));
    }
    else
    {
        ui->play->setIcon(QIcon(":/images/play3.png"));
    }
}

void QQMusic::onPlaybackModeChanged(QMediaPlaylist::PlaybackMode playbackMode)
{
    if(playbackMode == QMediaPlaylist::Loop)
    {
        ui->playMode->setIcon(QIcon(":/images/list_play.png"));
    }
    else if(playbackMode == QMediaPlaylist::Random)
    {
        ui->playMode->setIcon(QIcon(":/images/shuffle_2.png"));
    }
    else if(playbackMode == QMediaPlaylist::CurrentItemInLoop)
    {
        ui->playMode->setIcon(QIcon(":/images/single_play.png"));
    }
    else
    {
        qDebug()<<"暂不支持该模式";
    }
}

void QQMusic::onCurrentIndexChanged(int index)
{
    currentIndex = index;
    // 由于CommonPage页面中的歌曲和正在播放的播放列表中的歌曲的先后次序是相同的
    // 知道歌曲在playlist中的索引之后，直接到CommonPage中获取
    // 注意：playlist中的歌曲就是根据CommonPage中的musicOfPage加载
    QString musicId = currentPage->getMusicIdByIndex(index);

    // 本意是想要通过索引拿到歌曲，然后修改歌曲的isHistory属性
    auto it = musicList.findMusicByMusicId(musicId);
    if(it != musicList.end())
    {
        it->setIsHistory(true);
    }

    ui->recentPage->reFresh(musicList);
}

void QQMusic::onDurationChanged(qint64 duration)
{
    totalTime = duration;
    // duration为歌曲的总时长,单位为毫秒
    // 需要将整形的总时长转换为min:sec
    // 分：duration/1000/60;
    // 秒：duration/1000%60;
    ui->totalTime->setText(QString("%1:%2").arg(duration/1000/60, 2, 10, QChar('0'))
                           .arg(duration/1000%60, 2, 10, QChar('0')));
}

void QQMusic::onPositionChanged(qint64 position)
{
    // 更新当前播放时间
    ui->currentTime->setText(QString("%1:%2").arg(position/1000/60, 2, 10, QChar('0'))
                                             .arg(position/1000%60, 2, 10, QChar('0')));

    // 更新进度条的位置
    ui->progressBar->setStep(position/(float)totalTime);

    // 在歌词界面同步显示歌词
    if(currentIndex >= 0)
    {
        lrcPage->showLrcWordLine(position);
    }
}

void QQMusic::onMusicSliderChanged(float ration)
{
    // 根据进度条与总宽度的比率，更新当前播放时间
    qint64 duration = totalTime*ration;
    ui->currentTime->setText(QString("%1:%2").arg(duration/1000/60, 2, 10, QChar('0'))
                                             .arg(duration/1000%60, 2, 10, QChar('0')));

    // 时间修改了之后，播放时间也需要修改
    player->setPosition(duration);
}

void QQMusic::onMetaDataAvailableChanged(bool available)
{
    (int)(available);
    // 歌曲名称、歌曲作者直接到Musci对象中获取
    // 此时需要知道媒体源在播放列表中的索引
    QString musicId = currentPage->getMusicIdByIndex(currentIndex);
    auto it = musicList.findMusicByMusicId(musicId);

    QString musicName("未知歌曲");
    QString musicSinger("歌手位置");
    if(it != musicList.end())
    {
        musicName = it->getMusicName();
        musicSinger = it->getMusicSinger();
    }

    ui->musicName->setText(musicName);
    ui->musicSinger->setText(musicSinger);

    // 获取封面图，通过元数据来获取
    QVariant coverimage = player->metaData("ThumbnailImage");
    if(coverimage.isValid())
    {
        QImage image = coverimage.value<QImage>();
        ui->musicCover->setPixmap(QPixmap::fromImage(image));
        currentPage->setMusicImage(QPixmap::fromImage(image));
    }
    else
    {
        qDebug()<<"歌曲没有封面图";
        // 可以设置默认图片
        QString path = ":/images/rec/001.png";
        ui->musicCover->setPixmap(path);
        currentPage->setMusicImage(path);
    }
    ui->musicCover->setScaledContents(true);

    // 解析歌曲的LRC歌词
    if(it != musicList.end())
    {
        // 获取lrc文件的路径
        QString lrcPath = it->getLrcFilePath();

        // 解析lrc文件
        lrcPage->parseLrcFile(lrcPath);
    }
}



void QQMusic::on_skin_clicked()
{
    // 换肤
    // 类似：给窗口更新背景颜色 或者 更换背景图片
    QMessageBox::information(this, "温馨提示", "换肤功能小哥哥正在紧急支持中...");
}

void QQMusic::on_min_clicked()
{
    showMinimized();
}

void QQMusic::on_playUp_clicked()
{

}


void QQMusic::on_play_clicked()
{

}

