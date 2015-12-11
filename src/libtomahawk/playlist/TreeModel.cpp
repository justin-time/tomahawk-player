/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012,      Leo Franchi <lfranchi@kde.org>
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

#include "TreeModel.h"

#include <QMimeData>

#include "Pipeline.h"
#include "Source.h"
#include "SourceList.h"
#include "audio/AudioEngine.h"
#include "database/DatabaseCommand_AllAlbums.h"
#include "database/DatabaseCommand_AllTracks.h"
#include "database/Database.h"
#include "AlbumPlaylistInterface.h"
#include "PlayableItem.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Closure.h"
#include "utils/Logger.h"

using namespace Tomahawk;


TreeModel::TreeModel( QObject* parent )
    : PlayableModel( parent )
    , m_mode( DatabaseMode )
{
    PlayableModel::setIcon( TomahawkUtils::tinted( TomahawkUtils::defaultPixmap(
                                                       TomahawkUtils::DefaultCollection,
                                                       TomahawkUtils::Original,
                                                       // big enough for the ViewPage header on retina
                                                       QSize( 256, 256 ) ), Qt::white ) );

    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ), Qt::DirectConnection );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ), Qt::DirectConnection );
}


TreeModel::~TreeModel()
{
    tDebug() << Q_FUNC_INFO;
}


void
TreeModel::setMode( ModelMode mode )
{
    clear();
    m_mode = mode;
    emit modeChanged( mode );
}


Tomahawk::collection_ptr
TreeModel::collection() const
{
    return m_collection;
}


void
TreeModel::getCover( const QModelIndex& index )
{
    PlayableItem* item = itemFromIndex( index );

    if ( !item->artist().isNull() && !item->artist()->coverLoaded() )
        item->artist()->cover( QSize( 0, 0 ) );
    else if ( !item->album().isNull() && !item->album()->coverLoaded() )
        item->album()->cover( QSize( 0, 0 ) );
}


bool
TreeModel::canFetchMore( const QModelIndex& parent ) const
{
    PlayableItem* parentItem = itemFromIndex( parent );

    if ( parentItem->fetchingMore() )
        return false;

    if ( !parentItem->artist().isNull() )
    {
        return true;
    }
    else if ( !parentItem->album().isNull() )
    {
        return true;
    }

    return false;
}


void
TreeModel::fetchMore( const QModelIndex& parent )
{
    PlayableItem* parentItem = itemFromIndex( parent );
    if ( !parentItem || parentItem->fetchingMore() )
        return;

    parentItem->setFetchingMore( true );
    if ( !parentItem->artist().isNull() )
    {
        tDebug() << Q_FUNC_INFO << "Loading Artist:" << parentItem->artist()->name();
        fetchAlbums( parentItem->artist() );
    }
    else if ( !parentItem->album().isNull() )
    {
        tDebug() << Q_FUNC_INFO << "Loading Album:" << parentItem->album()->artist()->name() << parentItem->album()->name() << parentItem->album()->id();
        addTracks( parentItem->album(), parent );
    }
    else
        Q_ASSERT( false );
}


void
TreeModel::addArtists( const artist_ptr& artist )
{
    if ( artist.isNull() )
        return;

    startLoading();

    QList<Tomahawk::artist_ptr> artists;
    artists << artist;
    onArtistsAdded( artists );
}


void
TreeModel::fetchAlbums( const artist_ptr& artist )
{
    startLoading();

    connect( artist.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                              SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ), Qt::UniqueConnection );

    const QModelIndex parent = indexFromArtist( artist );
    addAlbums( parent, artist->albums( m_mode, m_collection ) );
}


void
TreeModel::onAlbumsFound( const QList<Tomahawk::album_ptr>& albums, ModelMode mode )
{
    if ( m_mode != mode )
        return;

    Tomahawk::Artist* artist = qobject_cast< Tomahawk::Artist* >( sender() );
    if ( !artist )
        return;

    const artist_ptr artistp = artist->weakRef().toStrongRef();
    disconnect( artist, SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                this,     SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ) );

    const QModelIndex parent = indexFromArtist( artistp );
    addAlbums( parent, albums );
}


void
TreeModel::addAlbums( const QModelIndex& parent, const QList<Tomahawk::album_ptr>& albums )
{
    finishLoading();
    if ( !albums.count() )
        return;

    PlayableItem* parentItem = itemFromIndex( parent );

    QPair< int, int > crows;
    const int c = rowCount( parent );
    crows.first = c;
    crows.second = c + albums.count() - 1;

    emit beginInsertRows( parent, crows.first, crows.second );

    PlayableItem* albumitem = 0;
    foreach( const album_ptr& album, albums )
    {
        albumitem = new PlayableItem( album, parentItem );
        albumitem->index = createIndex( parentItem->children.count() - 1, 0, albumitem );
        connect( albumitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );

        getCover( albumitem->index );
    }

    emit endInsertRows();
}


void
TreeModel::addTracks( const album_ptr& album, const QModelIndex& parent )
{
    startLoading();

    onTracksAdded( album->tracks( m_mode, m_collection ), parent );
    NewClosure( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                const_cast<TreeModel*>(this), SLOT( addTracks( Tomahawk::album_ptr, QModelIndex ) ), album, parent );
}


void
TreeModel::addCollection( const collection_ptr& collection )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->nodeId();

    startLoading();

    m_collection = collection;

    Tomahawk::ArtistsRequest* req = m_collection->requestArtists();
    connect( dynamic_cast< QObject* >( req ), SIGNAL( artists( QList< Tomahawk::artist_ptr > ) ),
             this, SLOT( onArtistsAdded( QList< Tomahawk::artist_ptr > ) ), Qt::UniqueConnection );
    req->enqueue();

    setIcon( collection->bigIcon() );
    setTitle( collection->prettyName() );
    setDescription( collection->description() );
}


//void
//TreeModel::addFilteredCollection( const collection_ptr& collection, unsigned int amount, DatabaseCommand_AllArtists::SortOrder order )
//{
//    qDebug() << Q_FUNC_INFO << collection->name()
//                            << collection->source()->id()
//                            << collection->source()->nodeId()
//                            << amount << order;
//    DatabaseCommand_AllArtists* cmd = new DatabaseCommand_AllArtists( collection );
//    cmd->setLimit( amount );
//    cmd->setSortOrder( order );
//    cmd->setSortDescending( true );

//    connect( cmd, SIGNAL( artists( QList<Tomahawk::artist_ptr>, Tomahawk::collection_ptr ) ),
//                    SLOT( onArtistsAdded( QList<Tomahawk::artist_ptr>, Tomahawk::collection_ptr ) ) );

//    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );

//    if ( collection->source()->isLocal() )
//        setTitle( tr( "My Collection" ) );
//    else
//        setTitle( tr( "Collection of %1" ).arg( collection->source()->friendlyName() ) );
//}


void
TreeModel::onArtistsAdded( const QList<Tomahawk::artist_ptr>& artists )
{
    finishLoading();

    if ( artists.isEmpty() )
        return;

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + artists.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    PlayableItem* artistitem;
    foreach( const artist_ptr& artist, artists )
    {
        artistitem = new PlayableItem( artist, rootItem() );
        artistitem->index = createIndex( rootItem()->children.count() - 1, 0, artistitem );
        connect( artistitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
}


void
TreeModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const QModelIndex& parent )
{
    finishLoading();

    if ( !tracks.count() )
        return;

    PlayableItem* parentItem = itemFromIndex( parent );

    QPair< int, int > crows;
    int c = rowCount( parent );
    removeRows( 0, c, parent );

    crows.first = c;
    crows.second = c + tracks.count() - 1;

    emit beginInsertRows( parent, crows.first, crows.second );

    PlayableItem* item = 0;
    foreach( const query_ptr& query, tracks )
    {
        item = new PlayableItem( query, parentItem );
        item->index = createIndex( parentItem->children.count() - 1, 0, item );

        connect( item, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit selectRequest( index( 0, 0, parent ) );
}


QModelIndex
TreeModel::indexFromArtist( const Tomahawk::artist_ptr& artist ) const
{
    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        QModelIndex idx = index( i, 0, QModelIndex() );
        PlayableItem* item = itemFromIndex( idx );
        if ( item && item->artist() == artist )
        {
            return idx;
        }
    }

    tDebug() << Q_FUNC_INFO << "Could not find item for artist:" << artist->name();
    return QModelIndex();
}


QModelIndex
TreeModel::indexFromAlbum( const Tomahawk::album_ptr& album ) const
{
    QModelIndex artistIdx = indexFromArtist( album->artist() );
    for ( int i = 0; i < rowCount( artistIdx ); i++ )
    {
        QModelIndex idx = index( i, 0, artistIdx );
        PlayableItem* item = itemFromIndex( idx );
        if ( item && item->album() == album )
        {
            return idx;
        }
    }

    tDebug() << Q_FUNC_INFO << "Could not find item for album:" << album->name() << album->artist()->name();
    return QModelIndex();
}


QModelIndex
TreeModel::indexFromResult( const Tomahawk::result_ptr& result ) const
{
    QModelIndex albumIdx = indexFromAlbum( result->track()->albumPtr() );
    for ( int i = 0; i < rowCount( albumIdx ); i++ )
    {
        QModelIndex idx = index( i, 0, albumIdx );
        PlayableItem* item = itemFromIndex( idx );
        // tDebug() << Q_FUNC_INFO << item->result()->toString();
        if ( item && item->result() == result )
        {
            return idx;
        }
    }

    tDebug() << Q_FUNC_INFO << "Could not find item for result:" << result->toString();
    return QModelIndex();
}


QModelIndex
TreeModel::indexFromQuery( const Tomahawk::query_ptr& query ) const
{
    QModelIndex albumIdx = indexFromAlbum( query->queryTrack()->albumPtr() );
    for ( int i = 0; i < rowCount( albumIdx ); i++ )
    {
        QModelIndex idx = index( i, 0, albumIdx );
        PlayableItem* item = itemFromIndex( idx );
        if ( item && item->result() && item->result()->track()->equals( query->track() ) )
        {
            return idx;
        }
    }

    tDebug() << Q_FUNC_INFO << "Could not find item for query:" << query->toString();
    return QModelIndex();
}


PlayableItem*
TreeModel::itemFromResult( const Tomahawk::result_ptr& result ) const
{
    QModelIndex albumIdx = indexFromAlbum( result->track()->albumPtr() );
    for ( int i = 0; i < rowCount( albumIdx ); i++ )
    {
        QModelIndex idx = index( i, 0, albumIdx );
        PlayableItem* item = itemFromIndex( idx );
        if ( item && item->result() == result )
        {
            return item;
        }
    }

    tDebug() << Q_FUNC_INFO << "Could not find item for result:" << result->toString();
    return 0;
}
