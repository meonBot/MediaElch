#ifndef GLOBALS_H
#define GLOBALS_H

#include <QDate>
#include <QDebug>
#include <QImage>
#include <QMetaType>
#include <QString>
#include <QUrl>
#include <QVariant>

class Movie;
class TvShowEpisode;

namespace Constants {
    const int MovieFileSearcherProgressMessageId   = 10000;
    const int MovieWidgetProgressMessageId         = 10001;
    const int TvShowSearcherProgressMessageId      = 10002;
    const int TvShowWidgetProgressMessageId        = 10003;
    const int TvShowWidgetSaveProgressMessageId    = 10004;
    const int ConcertFileSearcherProgressMessageId = 10005;
    const int MovieProgressMessageId               = 20000;
    const int TvShowProgressMessageId              = 40000;
    const int EpisodeProgressMessageId             = 60000;
    const int ConcertProgressMessageId             = 80000;
}

namespace TvShowRoles {
    const int Type = Qt::UserRole+1;
    const int ParentId = Qt::UserRole+2;
    const int Id = Qt::UserRole+3;
    const int EpisodeCount = Qt::UserRole+4;
    const int HasChanged = Qt::UserRole+5;
    const int IsNew = Qt::UserRole+6;
}

namespace MediaCenterInterfaces {
    const int XbmcXml    = 1;
    const int XbmcMysql  = 2;
    const int XbmcSqlite = 3;
}

namespace MediaCenterFeatures {
    const int EditTvShowEpisodeCertification = 1;
    const int EditTvShowEpisodeShowTitle     = 2;
    const int EditTvShowEpisodeNetwork       = 3;
    const int HandleMovieSetImages           = 4;
    const int EditConcertRating              = 5;
    const int EditConcertTagline             = 6;
    const int EditConcertCertification       = 7;
    const int EditConcertTrailer             = 8;
    const int EditConcertWatched             = 9;
}

enum MainActions {
    ActionSearch, ActionSave, ActionSaveAll, ActionFilterWidget, ActionExport
};

enum MainWidgets {
    WidgetMovies, WidgetMovieSets, WidgetTvShows, WidgetConcerts, WidgetGenres, WidgetCertifications
};

/**
 * @brief The Actor struct
 */
struct Actor {
    QString name;
    QString role;
    QString thumb;
    QImage image;
    bool imageHasChanged;
};
Q_DECLARE_METATYPE(Actor*);
Q_DECLARE_METATYPE(Movie*);
Q_DECLARE_METATYPE(QString*);

/**
 * @brief The ScraperSearchResult struct
 */
struct ScraperSearchResult {
    QString id;
    QString name;
    QDate released;
};

/**
 * @brief The Poster struct
 */
struct Poster {
    QString id;
    QUrl originalUrl;
    QUrl thumbUrl;
    QSize originalSize;
};

enum ImageType {
    TypePoster, TypeBackdrop, TypeBanner, TypeActor, TypeSeasonPoster, TypeShowThumbnail
};

enum TvShowType {
    TypeTvShow, TypeEpisode, TypeSeason
};

/**
 * @brief The SettingsDir struct
 */
struct SettingsDir {
    QString path;
    QString mediaCenterPath;
    bool separateFolders;
};

enum SettingsDirType {
    DirTypeMovies, DirTypeTvShows, DirTypeConcerts
};

namespace ImageDialogType {
    const int MoviePoster     = 1;
    const int MovieBackdrop   = 2;
    const int TvShowPoster    = 3;
    const int TvShowBackdrop  = 4;
    const int TvShowThumb     = 5;
    const int TvShowSeason    = 6;
    const int TvShowBanner    = 7;
    const int ConcertPoster   = 8;
    const int ConcertBackdrop = 9;
}

namespace MovieScraperInfos {
    const int Title         = 1;
    const int Tagline       = 2;
    const int Rating        = 3;
    const int Released      = 4;
    const int Runtime       = 5;
    const int Certification = 6;
    const int Trailer       = 7;
    const int Overview      = 8;
    const int Poster        = 9;
    const int Backdrop      = 10;
    const int Actors        = 11;
    const int Genres        = 12;
    const int Studios       = 13;
    const int Countries     = 14;
}

namespace ConcertScraperInfos {
    const int Title         = 1;
    const int Tagline       = 2;
    const int Rating        = 3;
    const int Released      = 4;
    const int Runtime       = 5;
    const int Certification = 6;
    const int Trailer       = 7;
    const int Overview      = 8;
    const int Poster        = 9;
    const int Backdrop      = 10;
    const int Genres        = 11;
}

// Debugging
#if defined( Q_CC_GNU )
#    if defined( qDebug )
#        undef qDebug
#    endif
#    define DEBUG_FUNCTION_NAME  QString("%1").arg(__PRETTY_FUNCTION__, -70, QLatin1Char(' '))
#    define qDebug() qDebug() << QString(DEBUG_FUNCTION_NAME + " :")
#endif

#endif // GLOBALS_H