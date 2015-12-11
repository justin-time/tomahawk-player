/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef TOMAHAWKARTIST_H
#define TOMAHAWKARTIST_H

#include <QFuture>
#include <QPixmap>

#include "TrackData.h"
#include "Typedefs.h"
#include "DllMacro.h"
#include "Query.h"

namespace Tomahawk
{

class IdThreadWorker;

class DLLEXPORT Artist : public QObject
{
Q_OBJECT

public:
    static artist_ptr get( const QString& name, bool autoCreate = false );
    static artist_ptr get( unsigned int id, const QString& name );

    Artist( unsigned int id, const QString& name );
    explicit Artist( const QString& name );
    virtual ~Artist();

    unsigned int id() const;
    QString name() const { return m_name; }
    QString sortname() const { return m_sortname; }

    QList<album_ptr> albums( ModelMode mode = Mixed, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() ) const;
    QList<artist_ptr> similarArtists() const;

    QList<Tomahawk::query_ptr> tracks( ModelMode mode = Mixed, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() );
    Tomahawk::playlistinterface_ptr playlistInterface( ModelMode mode, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() );

    void loadStats();
    QList< Tomahawk::PlaybackLog > playbackHistory( const Tomahawk::source_ptr& source = Tomahawk::source_ptr() ) const;
    void setPlaybackHistory( const QList< Tomahawk::PlaybackLog >& playbackData );
    unsigned int playbackCount( const Tomahawk::source_ptr& source = Tomahawk::source_ptr() ) const;

    unsigned int chartPosition() const;
    unsigned int chartCount() const;

    QString biography() const;

    QPixmap cover( const QSize& size, bool forceLoad = true ) const;
    bool coverLoaded() const { return m_coverLoaded; }

    Tomahawk::playlistinterface_ptr playlistInterface();

    QWeakPointer< Tomahawk::Artist > weakRef() { return m_ownRef; }
    void setWeakRef( const QWeakPointer< Tomahawk::Artist >& weakRef ) { m_ownRef = weakRef; }

    void loadId( bool autoCreate );

public slots:
    void deleteLater();

signals:
    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks, Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );
    void albumsAdded( const QList<Tomahawk::album_ptr>& albums, Tomahawk::ModelMode mode );

    void updated();
    void coverChanged();
    void similarArtistsLoaded();
    void biographyLoaded();
    void statsLoaded();

private slots:
    void onArtistStatsLoaded( unsigned int plays, unsigned int chartPos, unsigned int chartCount );
    void onTracksLoaded( Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );
    void onAlbumsFound( const QList<Tomahawk::album_ptr>& albums, const QVariant& collectionIsNull = QVariant( false ) );

    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( QString target );

private:
    Artist();
    QString infoid() const;

    void setIdFuture( QFuture<unsigned int> idFuture );

    mutable bool m_waitingForFuture;
    mutable QFuture<unsigned int> m_idFuture;
    mutable unsigned int m_id;

    QString m_name;
    QString m_sortname;

    bool m_coverLoaded;
    mutable bool m_coverLoading;
    QHash<Tomahawk::ModelMode, bool> m_albumsLoaded;
    bool m_simArtistsLoaded;
    bool m_biographyLoaded;

    mutable QString m_uuid;
    mutable int m_infoJobs;

    QList<Tomahawk::album_ptr> m_databaseAlbums;
    QList<Tomahawk::album_ptr> m_officialAlbums;
    QList<Tomahawk::artist_ptr> m_similarArtists;
    QString m_biography;

    QList< PlaybackLog > m_playbackHistory;
    unsigned int m_chartPosition;
    unsigned int m_chartCount;

    mutable QByteArray m_coverBuffer;
    mutable QPixmap* m_cover;

    QHash< Tomahawk::ModelMode, QHash< Tomahawk::collection_ptr, Tomahawk::playlistinterface_ptr > > m_playlistInterface;

    QWeakPointer< Tomahawk::Artist > m_ownRef;

    static QHash< QString, artist_wptr > s_artistsByName;
    static QHash< unsigned int, artist_wptr > s_artistsById;

    friend class IdThreadWorker;
};

} // ns

Q_DECLARE_METATYPE( Tomahawk::artist_ptr )

#endif
