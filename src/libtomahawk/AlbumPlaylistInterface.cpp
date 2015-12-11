/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "AlbumPlaylistInterface.h"

#include "utils/Logger.h"
#include "collection/TracksRequest.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_AllTracks.h"

#include "Artist.h"
#include "Pipeline.h"
#include "Query.h"
#include "Result.h"
#include "Source.h"
#include "SourceList.h"

#include <QDateTime>


using namespace Tomahawk;


AlbumPlaylistInterface::AlbumPlaylistInterface( Tomahawk::Album* album, Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection )
    : Tomahawk::PlaylistInterface()
    , m_currentItem( 0 )
    , m_infoSystemLoaded( false )
    , m_databaseLoaded( false )
    , m_mode( mode )
    , m_collection( collection )
    , m_album( QPointer< Tomahawk::Album >( album ) )
    , m_lastQueryTimestamp( 0 )
{
    if ( m_collection )
    {
        connect( collection.data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
    }
}


AlbumPlaylistInterface::~AlbumPlaylistInterface()
{
    m_album = 0;
}


void
AlbumPlaylistInterface::setCurrentIndex( qint64 index )
{
    if ( index >= 0 && index < m_queries.size() &&
         !m_queries.at( index ).isNull() && m_queries.at( index )->results().size() > 0 )
    {
        PlaylistInterface::setCurrentIndex( index );
        m_currentItem = m_queries.at( index )->results().first();
    }
}


qint64
AlbumPlaylistInterface::siblingIndex( int itemsAway, qint64 rootIndex ) const
{
    qint64 p = m_currentIndex;
    if ( rootIndex >= 0 )
        p = rootIndex;

    p += itemsAway;

    if ( p < 0 )
        return -1;

    if ( p >= m_queries.count() )
        return -1;

    return p;
}


result_ptr
AlbumPlaylistInterface::currentItem() const
{
    return m_currentItem;
}


bool
AlbumPlaylistInterface::setCurrentTrack( unsigned int albumpos )
{
    Q_UNUSED( albumpos );
    Q_ASSERT( false );
    return false;

/*    albumpos--;
    if ( ( int ) albumpos >= m_queries.count() )
        return false;

    m_currentTrack = albumpos;
    m_currentItem = m_queries.at( albumpos )->results().first();
    return true;*/
}


QList< Tomahawk::query_ptr >
AlbumPlaylistInterface::tracks() const
{
    if ( m_queries.isEmpty() && m_album && QDateTime::currentMSecsSinceEpoch() - m_lastQueryTimestamp > 10000 /*10s*/ )
    {
        if ( ( m_mode == Mixed || m_mode == InfoSystemMode ) && !m_infoSystemLoaded )
        {
            Tomahawk::InfoSystem::InfoStringHash artistInfo;
            artistInfo["artist"] = m_album.data()->artist()->name();
            artistInfo["album"] = m_album.data()->name();

            Tomahawk::InfoSystem::InfoRequestData requestData;
            requestData.caller = id();
            requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
            requestData.type = Tomahawk::InfoSystem::InfoAlbumSongs;
            requestData.timeoutMillis = 0;
            requestData.allSources = true;
            Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

            connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                    SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                    SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

            connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                    SIGNAL( finished( QString ) ),
                    SLOT( infoSystemFinished( QString ) ) );

            const_cast< int& >( m_lastQueryTimestamp ) = QDateTime::currentMSecsSinceEpoch();
        }
        else if ( m_mode == DatabaseMode && !m_databaseLoaded && !isFinished() )
        {
            if ( m_collection.isNull() ) //we do a dbcmd directly, for the SuperCollection I guess?
            {
                DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
                cmd->setAlbum( m_album->weakRef() );
                cmd->setSortOrder( DatabaseCommand_AllTracks::AlbumPosition );
                connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                                SLOT( onTracksLoaded( QList<Tomahawk::query_ptr> ) ) );
                Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
            }
            else
            {
                Tomahawk::album_ptr ap = Album::get( m_album->id(), m_album->name(), m_album->artist() );

                Tomahawk::TracksRequest* cmd = m_collection->requestTracks( ap );
                connect( dynamic_cast< QObject* >( cmd ), SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ),
                         this, SLOT( onTracksLoaded( QList<Tomahawk::query_ptr> ) ), Qt::UniqueConnection );

                cmd->enqueue();
            }
            const_cast< int& >( m_lastQueryTimestamp ) = QDateTime::currentMSecsSinceEpoch();
        }
    }

    return m_queries;
}


void
AlbumPlaylistInterface::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != id() )
        return;

    switch ( requestData.type )
    {
        case Tomahawk::InfoSystem::InfoAlbumSongs:
        {
            QVariantMap returnedData = output.value< QVariantMap >();
            if ( !returnedData.isEmpty() )
            {
                Tomahawk::InfoSystem::InfoStringHash inputInfo;
                inputInfo = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();

                QStringList tracks = returnedData[ "tracks" ].toStringList();
                QList<query_ptr> ql;

                //TODO: Figure out how to do this with a multi-disk album without breaking the
                //      current behaviour. I just know too little about InfoSystem to deal with
                //      it right now, I've only taken the liberty of adding Query::setDiscNumber
                //      which should make this easier. --Teo 11/2011
                unsigned int trackNo = 1;

                foreach ( const QString& trackName, tracks )
                {
                    track_ptr track = Track::get( inputInfo[ "artist" ], trackName, inputInfo[ "album" ], QString(), 0, QString(), trackNo++ );
                    query_ptr query = Query::get( track );
                    if ( query )
                        ql << query;
                }
                Pipeline::instance()->resolve( ql );

                m_queries << ql;
            }

            break;
        }

        default:
        {
            Q_ASSERT( false );
            break;
        }
    }

    if ( !m_queries.isEmpty() )
    {
        infoSystemFinished( id() );
    }
}


void
AlbumPlaylistInterface::infoSystemFinished( const QString& infoId )
{
    if ( infoId != id() )
        return;

    m_infoSystemLoaded = true;
    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                this, SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );
    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ),
                this, SLOT( infoSystemFinished( QString ) ) );

    // Add !m_finished check to not endlessly reload on an empty album.
    if ( m_queries.isEmpty() && m_mode == Mixed && !isFinished() )
    {
        if ( m_collection.isNull() ) //we do a dbcmd directly, for the SuperCollection I guess?
        {
            DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
            cmd->setAlbum( m_album->weakRef() );
            cmd->setSortOrder( DatabaseCommand_AllTracks::AlbumPosition );
            connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                            SLOT( onTracksLoaded( QList<Tomahawk::query_ptr> ) ) );
            Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
        }
        else
        {
            Tomahawk::album_ptr ap = Album::get( m_album->id(), m_album->name(), m_album->artist() );

            Tomahawk::TracksRequest* cmd = m_collection->requestTracks( ap );
            connect( dynamic_cast< QObject* >( cmd ), SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ),
                     this, SLOT( onTracksLoaded( QList<Tomahawk::query_ptr> ) ), Qt::UniqueConnection );

            cmd->enqueue();
        }
    }
    else
    {
        finishLoading();
        emit tracksLoaded( m_mode, m_collection );
    }
}


void
AlbumPlaylistInterface::onTracksLoaded( const QList< query_ptr >& tracks )
{
    if ( m_collection.isNull() )
    {
        m_databaseLoaded = true;
        m_queries << filterTracks( tracks );
    }
    else
        m_queries << tracks;

    finishLoading();
    emit tracksLoaded( m_mode, m_collection );
}


void
AlbumPlaylistInterface::onCollectionChanged()
{
//    tDebug() << Q_FUNC_INFO << m_album->name();
    if ( m_mode == Tomahawk::DatabaseMode )
    {
        startLoading();
        m_databaseLoaded = false;
        m_queries.clear();
    }
}


qint64
AlbumPlaylistInterface::indexOfResult( const Tomahawk::result_ptr& result ) const
{
    int i = 0;
    foreach ( const Tomahawk::query_ptr& query, m_queries )
    {
        if ( query->numResults() && query->results().contains( result ) )
            return i;

        i++;
    }

    return -1;
}


qint64
AlbumPlaylistInterface::indexOfQuery( const Tomahawk::query_ptr& query ) const
{
    int i = 0;
    foreach ( const Tomahawk::query_ptr& q, m_queries )
    {
        if ( q->equals( query ) )
            return i;

        i++;
    }

    return -1;
}


query_ptr
AlbumPlaylistInterface::queryAt( qint64 index ) const
{
    if ( index >= 0 && index < m_queries.count() )
    {
        return m_queries.at( index );
    }

    return Tomahawk::query_ptr();
}


result_ptr
AlbumPlaylistInterface::resultAt( qint64 index ) const
{
    Tomahawk::query_ptr query = queryAt( index );
    if ( query && query->numResults() )
        return query->results().first();

    return Tomahawk::result_ptr();
}
