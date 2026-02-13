#include "music.h"
#include <QUuid>
#include <QMediaPlayer>
#include <QDebug>
#include <QCoreApplication>

#include <QSqlQuery>
#include <QSqlError>

Music::Music()
    : isLike(false)
    , isHistory(false)
{

}

Music::Music(QUrl url)
    : isLike(false)
    , isHistory(false)
    , musicUrl(url)
{
    // 读取url对应的歌曲文件的信息，解析出元数据
    // 歌曲名称、歌曲作者、歌曲专辑、歌曲持续时长
    musicId = QUuid::createUuid().toString();
    parseMediaMetaMusic();
}

void Music::setMusicId(const QString &musicId)
{
    this->musicId = musicId;
}

void Music::setMusicName(const QString& musicName)
{
    this->musicName = musicName;
}

void Music::setMusicSinger(const QString& musicSinger)
{
    this->musicSinger = musicSinger;
}

void Music::setMusicAlbum(const QString& musicAlbumn)
{
    this->musicAlbumn = musicAlbumn;
}

void Music::setMusicDuration(qint64 duration)
{
    this->duration = duration;
}
void Music::setIsLike(bool isLike)
{
    this->isLike = isLike;
}

void Music::setIsHistory(bool isHistory)
{
    this->isHistory = isHistory;
}

void Music::setMusicUrl(QUrl musicUrl)
{
    this->musicUrl = musicUrl;
}

QString Music::getMusicName()const
{
    return musicName;
}

QString Music::getMusicSinger()const
{
    return musicSinger;
}

QString Music::getMusicAlbum()const
{
    return musicAlbumn;
}

qint64 Music::getMusicDuration()const
{
    return duration;
}

bool Music::getIsLike()const
{
    return isLike;
}

bool Music::getIsHistory()const
{
    return isHistory;
}

QUrl Music::getMusicUrl()const
{
    return musicUrl;
}

QString Music::getMusicId() const
{
    return musicId;
}

QString Music::getLrcFilePath() const
{
    // D:/musics/2002年的第一场学.mp3  歌曲文件路径
    // D:/musics/2022年的第一场学.lrc  歌词文件
    QString lrcPath = musicUrl.toLocalFile();
    lrcPath.replace(".mp3", ".lrc");
    lrcPath.replace(".flac", ".lrc");
    lrcPath.replace(".mpga", ".lrc");

    return lrcPath;
}

void Music::InsertMusicToDB()
{
    // 1. 检测music是否在数据库中存在
    QSqlQuery query;
    // query.prepare("SELECT * FROM MusicInfo WHERE musicId = ?");   // 效率低，因为要返回复合条件记录的所有列

    // 现在只需要检测musicId的歌曲是否存在
    // query.prepare("SELECT id FROM MusicInfo WHERE musicId = ?");    // 原表中有9列，现在只需要返回1列：id


    // 在MusicInfo表中查询MusicId等于指定值的歌曲，如果歌曲存在，返回记录中只包含一个1
    // 假设musicId的歌曲在MusicInfo中存在3个
    // SELECT 1 FROM MusicInfo WHERE musicId = ? 返回结果：
    // 1
    // 1
    // 1
    // query.prepare("SELECT 1 FROM MusicInfo WHERE musicId = ?");

    // 当SELECT 1...查询到结果后，我们需要知道是否存在
    // SELECT EXISTS(子查询) ： 子查询中如果有记录，SELECT EXISTS返回TRUE
    //                       如果子查询中没有满足条件的记录， SELECT EXISTS返回FALSE

    query.prepare("SELECT EXISTS (SELECT 1 FROM MusicInfo WHERE musicId = ?)");
    query.addBindValue(musicId);
    if(!query.exec())
    {
        qDebug()<<"查询失败: "<<query.lastError().text();
        return;
    }

    if(query.next())
    {
        bool isExists = query.value(0).toBool();
        if(isExists)
        {
            // musicId的歌曲已经存在
            // 2. 存在：不需要再插入musci对象，此时只需要将isLike和isHistory属性进行更新
            query.prepare("UPDATE MusicInfo SET isLike = ?, isHistory = ? WHERE musicId = ?");
            query.addBindValue(isLike? 1 : 0);
            query.addBindValue(isHistory? 1 : 0);
            query.addBindValue(musicId);
            if(!query.exec())
            {
                qDebug()<<"更新失败: "<<query.lastError().text();
            }

            qDebug()<<"更新music信息: "<<musicName<<" "<<musicId;
        }
        else
        {
            // 3. 不存在：直接将music的属性信息插入数据库
            query.prepare("INSERT INTO MusicInfo(musicId, musicName, musicSinger, albumName, musicUrl,\
                          duration, isLike, isHistory)\
                    VALUES(?,?,?,?,?,?,?,?)");
            query.addBindValue(musicId);
            query.addBindValue(musicName);
            query.addBindValue(musicSinger);
            query.addBindValue(musicAlbumn);
            query.addBindValue(musicUrl.toLocalFile());
            query.addBindValue(duration);
            query.addBindValue(isLike ? 1 : 0);
            query.addBindValue(isHistory? 1 : 0);

            if(!query.exec())
            {
                qDebug()<<"插入失败: "<<query.lastError().text();
                return;
            }

            qDebug()<<"插入music信息: "<<musicName<<" "<<musicId;
        }
    }

}

void Music::parseMediaMetaMusic()
{
    // 1. 创建媒体播放对象
    QMediaPlayer player;

    // 2. 设置媒体源，依靠setMedia方法解析元数据
    player.setMedia(musicUrl);

    // 3. 因为setMedia不会等待音乐的加载就立马返回
    //    因此在通过mediaMeta方法获取元数据时，需要保证媒体已经添加完毕
    //    可以通过isMetaDataAvailable检测数据是否有效
    //    如果该方法返回true，说明加载完成
    while(!player.isMetaDataAvailable())
    {
        QCoreApplication::processEvents();
    }

    // 已经加载完成，可以获取有效的元数据
    if(player.isMetaDataAvailable())
    {
        musicName = player.metaData("Title").toString();
        musicSinger = player.metaData("Author").toString();
        musicAlbumn = player.metaData("AlbumTitle").toString();
        duration = player.metaData("Duration").toLongLong();

        QString fileName = musicUrl.fileName();
        // 找-的位置
        int index = fileName.indexOf('-');

        // musicName为""的处理
        if(musicName.isEmpty())
        {
            if(index != -1)
            {
                // "2002年的第一场雪 - 刀郎.mp3"
                musicName = fileName.mid(0, index).trimmed();
            }
            else
            {
                // "2002年的第一场雪.mp3"
                musicName = fileName.mid(0, fileName.indexOf('.')).trimmed();
            }
        }

        // 作者为空
        if(musicSinger.isEmpty())
        {
            if(index != -1)
            {
                musicSinger = fileName.mid(index+1, fileName.indexOf('.')-index-1).trimmed();
            }
            else
            {
                musicSinger = "未知歌手";
            }
        }

        // 专辑名为空
        if(musicAlbumn.isEmpty())
        {
            musicAlbumn = "未知专辑";
        }

        // "2002年的第一场雪 - 刀郎.mp3"
        qDebug()<<fileName<<musicSinger<<":"<<musicAlbumn<<":"<<duration;
    }
}





