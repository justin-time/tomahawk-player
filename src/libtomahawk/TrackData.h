/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRACKDATA_H
#define TRACKDATA_H

#include <QObject>
#include <QList>
#include <QFuture>
#include <QVariant>

#include "infosystem/InfoSystem.h"
#include "DllMacro.h"
#include "PlaybackLog.h"
#include "SocialAction.h"
#include "Typedefs.h"


namespace Tomahawk
{

class DatabaseCommand_LogPlayback;
class DatabaseCommand_PlaybackHistory;
class IdThreadWorker;

class DLLEXPORT TrackData : public QObject
{
Q_OBJECT

public:
    enum DescriptionMode
    { Detailed = 0, Short = 1 };

    static trackdata_ptr get( unsigned int id, const QString& artist, const QString& track );

    virtual ~TrackData();

    QString id() const;

    QString toString() const;
    Tomahawk::query_ptr toQuery();

    const QString& artistSortname() const { return m_artistSortname; }
    const QString& trackSortname() const { return m_trackSortname; }

    QWeakPointer< Tomahawk::TrackData > weakRef() { return m_ownRef; }
    void setWeakRef( QWeakPointer< Tomahawk::TrackData > weakRef ) { m_ownRef = weakRef; }

    QString artist() const { return m_artist; }
    QString track() const { return m_track; }

    int year() const { return m_year; }

    void setLoved( bool loved );
    bool loved();

    void share( const Tomahawk::source_ptr& source );

    unsigned int trackId() const;
    void loadId( bool autoCreate ) const;

    void loadAttributes();
    QVariantMap attributes() const { return m_attributes; }
    void setAttributes( const QVariantMap& map ) { m_attributes = map; updateAttributes(); }

    void loadSocialActions( bool force = false );
    QList< Tomahawk::SocialAction > allSocialActions() const;
    QList< Tomahawk::SocialAction > socialActions( const QString& actionName, const QVariant& value = QVariant(), bool filterDupeSourceNames = false );
    void setAllSocialActions( const QList< Tomahawk::SocialAction >& socialActions );

    void loadStats();
    QList< Tomahawk::PlaybackLog > playbackHistory( const Tomahawk::source_ptr& source = Tomahawk::source_ptr() ) const;
    void setPlaybackHistory( const QList< Tomahawk::PlaybackLog >& playbackData );
    unsigned int playbackCount( const Tomahawk::source_ptr& source = Tomahawk::source_ptr() );

    unsigned int chartPosition() const;
    unsigned int chartCount() const;

    QList<Tomahawk::query_ptr> similarTracks() const;
    QStringList lyrics() const;

public slots:
    void deleteLater();

signals:
    void attributesLoaded();
    void socialActionsLoaded();
    void statsLoaded();
    void similarTracksLoaded();
    void lyricsLoaded();

private slots:
    void onTrackStatsLoaded( unsigned int chartPos, unsigned int chartCount );

    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( QString target );

private:
    explicit TrackData( unsigned int id, const QString& artist, const QString& track );

    void setIdFuture( QFuture<unsigned int> future );

    void updateAttributes();
    void parseSocialActions();
    void updateSortNames();

    QString m_artist;
    QString m_track;
    QString m_artistSortname;
    QString m_trackSortname;

    int m_year;

    bool m_attributesLoaded;
    QVariantMap m_attributes;

    bool m_socialActionsLoaded;
    QHash< QString, QVariant > m_currentSocialActions;
    QList< SocialAction > m_allSocialActions;

    bool m_playbackHistoryLoaded;
    QList< PlaybackLog > m_playbackHistory;
    unsigned int m_chartPosition;
    unsigned int m_chartCount;

    bool m_simTracksLoaded;
    QList<Tomahawk::query_ptr> m_similarTracks;

    bool m_lyricsLoaded;
    QStringList m_lyrics;

    mutable int m_infoJobs;
    mutable QString m_uuid;

    mutable bool m_waitingForId;
    mutable QFuture<unsigned int> m_idFuture;
    mutable unsigned int m_trackId;

    QWeakPointer< Tomahawk::TrackData > m_ownRef;

    static QHash< QString, trackdata_wptr > s_trackDatasByName;
    static QHash< unsigned int, trackdata_wptr > s_trackDatasById;

    friend class IdThreadWorker;
    friend class DatabaseCommand_LogPlayback;
    friend class DatabaseCommand_PlaybackHistory;
};

} // namespace Tomahawk

Q_DECLARE_METATYPE( QList<Tomahawk::PlaybackLog> )
Q_DECLARE_METATYPE( Tomahawk::trackdata_ptr )

#endif // TRACKDATA_H
