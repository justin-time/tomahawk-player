/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011       Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "PlayableModel_p.h"

#include "audio/AudioEngine.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

#include "Artist.h"
#include "Album.h"
#include "Pipeline.h"
#include "PlayableItem.h"
#include "PlayableProxyModel.h"
#include "Result.h"
#include "Source.h"
#include "Typedefs.h"

#include <QDateTime>
#include <QMimeData>
#include <QTreeView>

using namespace Tomahawk;


void
PlayableModel::init()
{
    Q_D( PlayableModel );
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ), Qt::DirectConnection );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ), Qt::DirectConnection );

    d->header << tr( "Artist" ) << tr( "Title" ) << tr( "Composer" ) << tr( "Album" ) << tr( "Track" ) << tr( "Duration" )
              << tr( "Bitrate" ) << tr( "Age" ) << tr( "Year" ) << tr( "Size" ) << tr( "Origin" ) << tr( "Accuracy" ) << tr( "Name" );
}


PlayableModel::PlayableModel( QObject* parent, bool loading )
    : QAbstractItemModel( parent )
    , d_ptr( new PlayableModelPrivate( this, loading ) )
{
    init();
}


PlayableModel::PlayableModel( QObject* parent, PlayableModelPrivate* d )
    : QAbstractItemModel( parent )
    , d_ptr( d )

{
    init();
}



PlayableModel::~PlayableModel()
{
    Q_D( PlayableModel );
    tDebug() << Q_FUNC_INFO;
    delete d->rootItem;
}


QModelIndex
PlayableModel::createIndex( int row, int column, PlayableItem* item ) const
{
    return QAbstractItemModel::createIndex( row, column, item );
}


QModelIndex
PlayableModel::index( int row, int column, const QModelIndex& parent ) const
{
    Q_D( const PlayableModel );
    if ( !d->rootItem || row < 0 || column < 0 )
        return QModelIndex();

    PlayableItem* parentItem = itemFromIndex( parent );
    PlayableItem* childItem = parentItem->children.value( row );
    if ( !childItem )
        return QModelIndex();

    return createIndex( row, column, childItem );
}


int
PlayableModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    PlayableItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    return parentItem->children.count();
}


int
PlayableModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );

    return 12;
}


bool
PlayableModel::hasChildren( const QModelIndex& parent ) const
{
    Q_D( const PlayableModel );
    PlayableItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return false;

    if ( parentItem == d->rootItem )
        return true;

    return ( !parentItem->artist().isNull() || !parentItem->album().isNull() || !parentItem->source().isNull() );
}


QModelIndex
PlayableModel::parent( const QModelIndex& child ) const
{
    PlayableItem* entry = itemFromIndex( child );
    if ( !entry )
        return QModelIndex();

    PlayableItem* parentEntry = entry->parent();
    if ( !parentEntry )
        return QModelIndex();

    PlayableItem* grandparentEntry = parentEntry->parent();
    if ( !grandparentEntry )
        return QModelIndex();

    int row = grandparentEntry->children.indexOf( parentEntry );
    return createIndex( row, 0, parentEntry );
}


bool
PlayableModel::isReadOnly() const
{
    Q_D( const PlayableModel );
    return d->readOnly;
}


void
PlayableModel::setReadOnly( bool b )
{
    Q_D( PlayableModel );
    d->readOnly = b;
}


bool
PlayableModel::isLoading() const
{
    Q_D( const PlayableModel );
    return d->loading;
}


QString
PlayableModel::title() const
{
    Q_D( const PlayableModel );
    return d->title;
}


QVariant
PlayableModel::artistData( const artist_ptr& artist, int role ) const
{
    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    return artist->name();
}


QVariant
PlayableModel::albumData( const album_ptr& album, int role ) const
{
    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    return album->name();
}


QVariant
PlayableModel::queryData( const query_ptr& query, int column, int role ) const
{
    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    switch ( column )
    {
        case Artist:
            return query->track()->artist();
            break;

        case Name:
        case Track:
            return query->track()->track();
            break;

        case Album:
            return query->track()->album();
            break;

        case Composer:
            return query->track()->composer();
            break;

        case Duration:
            return TomahawkUtils::timeToString( query->track()->duration() );
            break;

        case AlbumPos:
        {
            const uint tPos = query->track()->albumpos();
            if ( tPos != 0 )
            {
                const uint discnumber = query->track()->discnumber();
                if ( query->track()->discnumber() == 0 )
                    return QString::number( tPos );
                else
                    return QString( "%1.%2" ).arg( discnumber )
                                             .arg( tPos );
            }
        }
        break;

        default:
            break;
    }
    if ( query->numResults() )
    {
        switch ( column )
        {
            case Bitrate:
                if ( query->results().first()->bitrate() > 0 )
                    return query->results().first()->bitrate();
                break;

            case Age:
                return TomahawkUtils::ageToString( QDateTime::fromTime_t( query->results().first()->modificationTime() ) );
                break;

            case Year:
                if ( query->results().first()->track()->year() != 0 )
                    return query->results().first()->track()->year();
                break;

            case Filesize:
                return TomahawkUtils::filesizeToString( query->results().first()->size() );
                break;

            case Origin:
                return query->results().first()->friendlySource();
                break;

            case Score:
            {
                float score;
                if ( query->results().first()->isOnline() )
                    score = query->results().first()->score();
                else
                    score = 0.0;

                return scoreText( score );
                break;
            }

            default:
                break;
        }
    }
    else
    {
        switch ( column )
        {
            case Score:
                if ( query->resolvingFinished() )
                    return scoreText( 0.0 );
                else
                    return scoreText( -1.0 );

            default:
                break;
        }
    }

    return QVariant();
}


QVariant
PlayableModel::data( const QModelIndex& index, int role ) const
{
    PlayableItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    if ( role == PlayableProxyModel::TypeRole )
    {
        if ( entry->result() )
        {
            return Tomahawk::TypeResult;
        }
        else if ( entry->query() )
        {
            return Tomahawk::TypeQuery;
        }
        else if ( entry->artist() )
        {
            return Tomahawk::TypeArtist;
        }
        else if ( entry->album() )
        {
            return Tomahawk::TypeAlbum;
        }
    }

    int column = index.column();
    if ( role < CoverIDRole && role >= Qt::UserRole )
    {
        // map role to column
        column = role - Qt::UserRole;
        role = Qt::DisplayRole;
    }

    switch ( role )
    {
        case Qt::TextAlignmentRole:
        {
            return QVariant( columnAlignment( index.column() ) );
            break;
        }

        default:
        {
            if ( !entry->query().isNull() )
            {
                return queryData( entry->query(), column, role );
            }
            else if ( !entry->artist().isNull() )
            {
                return artistData( entry->artist(), role );
            }
            else if ( !entry->album().isNull() )
            {
                return albumData( entry->album(), role );
            }
            break;
        }
    }

    return QVariant();
}


QVariant
PlayableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_D( const PlayableModel );
    Q_UNUSED( orientation );

    if ( role == Qt::DisplayRole && section >= 0 )
    {
        if ( section < d->header.count() )
            return d->header.at( section );
        else
            return tr( "Name" );
    }

    if ( role == Qt::TextAlignmentRole )
    {
        return QVariant( columnAlignment( section ) );
    }

    return QVariant();
}


void
PlayableModel::setCurrentIndex( const QModelIndex& index )
{
    Q_D( PlayableModel );

    const QModelIndex oldIndex = d->currentIndex;
    PlayableItem* oldEntry = itemFromIndex( d->currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }

    PlayableItem* entry = itemFromIndex( index );
    if ( index.isValid() && entry && !entry->query().isNull() )
    {
        d->currentIndex = index;
        d->currentUuid = entry->query()->id();
        entry->setIsPlaying( true );
    }
    else
    {
        d->currentIndex = QModelIndex();
        d->currentUuid = QString();
    }

    emit currentIndexChanged( d->currentIndex, oldIndex );
}


Qt::DropActions
PlayableModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


Qt::ItemFlags
PlayableModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( index );

    if ( index.isValid() && index.column() == 0 )
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}


QPersistentModelIndex
PlayableModel::currentItem()
{
    Q_D( PlayableModel );
    return d->currentIndex;
}


QID
PlayableModel::currentItemUuid()
{
    Q_D( PlayableModel );
    return d->currentUuid;
}


PlaylistModes::RepeatMode
PlayableModel::repeatMode() const
{
    return Tomahawk::PlaylistModes::NoRepeat;
}


QStringList
PlayableModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.mixed";
    return types;
}


QMimeData*
PlayableModel::mimeData( const QModelIndexList &indexes ) const
{
    QByteArray resultData;
    QDataStream resultStream( &resultData, QIODevice::WriteOnly );

    // lets try with artist only
    bool fail = false;
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        PlayableItem* item = itemFromIndex( i );
        if ( !item )
            continue;

        if ( !item->artist().isNull() )
        {
            const artist_ptr& artist = item->artist();
            resultStream << artist->name();
        }
        else
        {
            fail = true;
            break;
        }
    }
    if ( !fail )
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setData( "application/tomahawk.metadata.artist", resultData );
        return mimeData;
    }

    // lets try with album only
    fail = false;
    resultData.clear();
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        PlayableItem* item = itemFromIndex( i );
        if ( !item )
            continue;

        if ( !item->album().isNull() )
        {
            const album_ptr& album = item->album();
            resultStream << album->artist()->name();
            resultStream << album->name();
        }
        else
        {
            fail = true;
            break;
        }
    }
    if ( !fail )
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setData( "application/tomahawk.metadata.album", resultData );
        return mimeData;
    }

    // lets try with tracks only
    fail = false;
    resultData.clear();
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        PlayableItem* item = itemFromIndex( i );
        if ( !item )
            continue;

        if ( !item->result().isNull() )
        {
            const result_ptr& result = item->result();
            resultStream << qlonglong( &result );
        }
        else
        {
            fail = true;
            break;
        }
    }
    if ( !fail )
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setData( "application/tomahawk.result.list", resultData );
        return mimeData;
    }

    // Ok... we have to use mixed
    resultData.clear();
    QDataStream mixedStream( &resultData, QIODevice::WriteOnly );
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        PlayableItem* item = itemFromIndex( i );
        if ( !item )
            continue;

        if ( !item->artist().isNull() )
        {
            const artist_ptr& artist = item->artist();
            mixedStream << QString( "application/tomahawk.metadata.artist" ) << artist->name();
        }
        else if ( !item->album().isNull() )
        {
            const album_ptr& album = item->album();
            mixedStream << QString( "application/tomahawk.metadata.album" ) << album->artist()->name() << album->name();
        }
        else if ( !item->result().isNull() )
        {
            const result_ptr& result = item->result();
            mixedStream << QString( "application/tomahawk.result.list" ) << qlonglong( &result );
        }
        else if ( !item->query().isNull() )
        {
            const query_ptr& query = item->query();
            mixedStream << QString( "application/tomahawk.query.list" ) << qlonglong( &query );
        }
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData( "application/tomahawk.mixed", resultData );
    return mimeData;
}


void
PlayableModel::clear()
{
    Q_D( PlayableModel );
    setCurrentIndex( QModelIndex() );

    if ( rowCount( QModelIndex() ) )
    {
        finishLoading();

        emit beginResetModel();
        delete d->rootItem;
        d->rootItem = 0;
        d->rootItem = new PlayableItem( 0 );
        emit endResetModel();
    }
}


QList< query_ptr >
PlayableModel::queries() const
{
    Q_D( const PlayableModel );
    Q_ASSERT( d->rootItem );

    QList< query_ptr > tracks;
    foreach ( PlayableItem* item, d->rootItem->children )
    {
        tracks << item->query();
    }

    return tracks;
}


template <typename T>
void
PlayableModel::insertInternal( const QList< T >& items, int row, const QList< Tomahawk::PlaybackLog >& logs, const QModelIndex& parent )
{
    Q_D( PlayableModel );
    if ( !items.count() )
    {
        emit itemCountChanged( rowCount( QModelIndex() ) );

        finishLoading();
        return;
    }

    int c = row;
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + items.count() - 1;

    emit beginInsertRows( parent, crows.first, crows.second );

    int i = 0;
    PlayableItem* plitem;
    foreach ( const T& item, items )
    {
        PlayableItem* pItem = itemFromIndex( parent );
        plitem = new PlayableItem( item, pItem, row + i );
        plitem->index = createIndex( row + i, 0, plitem );

        if ( plitem->query() )
        {
            if ( !plitem->query()->playable() )
            {
                connect( plitem->query().data(), SIGNAL( playableStateChanged( bool ) ),
                         SLOT( onQueryBecamePlayable( bool ) ),
                         Qt::UniqueConnection );
            }
            if ( !plitem->query()->resolvingFinished() )
            {
                connect( plitem->query().data(), SIGNAL( resolvingFinished( bool ) ),
                         SLOT( onQueryResolved( bool ) ),
                         Qt::UniqueConnection );
            }
        }

        if ( logs.count() > i )
            plitem->setPlaybackLog( logs.at( i ) );

        i++;

/*        if ( item->id() == currentItemUuid() )
            setCurrentItem( plitem->index );*/

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit itemCountChanged( rowCount( QModelIndex() ) );

    emit selectRequest( index( 0, 0, parent ) );
    if ( parent.isValid() )
        emit expandRequest( parent );

    finishLoading();
}


bool
PlayableModel::removeRows( int row, int count, const QModelIndex& parent )
{
    tDebug() << Q_FUNC_INFO << row << count << parent;
    QList<QPersistentModelIndex> pil;
    for ( int i = row; i < row + count; i++ )
    {
        pil << index( i, 0, parent );
    }

    removeIndexes( pil );
    return true;
}


void
PlayableModel::remove( int row, bool moreToCome, const QModelIndex& parent )
{
    removeIndex( index( row, 0, parent ), moreToCome );
}


void
PlayableModel::removeIndex( const QModelIndex& index, bool moreToCome )
{
    Q_D( PlayableModel );
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "removeIndex",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QModelIndex, index),
                                   Q_ARG(bool, moreToCome) );
        return;
    }

    if ( index.column() > 0 )
        return;

    PlayableItem* item = itemFromIndex( index );
    if ( item )
    {
        if ( index == d->currentIndex )
            setCurrentIndex( QModelIndex() );

        emit beginRemoveRows( index.parent(), index.row(), index.row() );
        delete item;
        emit endRemoveRows();
    }

    if ( !moreToCome )
        emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
PlayableModel::removeIndexes( const QList<QModelIndex>& indexes )
{
    QList<QPersistentModelIndex> pil;
    foreach ( const QModelIndex& idx, indexes )
    {
        pil << idx;
    }

    removeIndexes( pil );
}


void
PlayableModel::removeIndexes( const QList<QPersistentModelIndex>& indexes )
{
    QList<QPersistentModelIndex> finalIndexes;
    foreach ( const QPersistentModelIndex index, indexes )
    {
        if ( index.column() > 0 )
            continue;
        finalIndexes << index;
    }

    for ( int i = 0; i < finalIndexes.count(); i++ )
    {
        removeIndex( finalIndexes.at( i ), i + 1 != finalIndexes.count() );
    }
}


PlayableItem*
PlayableModel::rootItem() const
{
    Q_D( const PlayableModel );
    return d->rootItem;
}


void
PlayableModel::onPlaybackStarted( const Tomahawk::result_ptr result )
{
    Q_D( PlayableModel );
    PlayableItem* oldEntry = itemFromIndex( d->currentIndex );
    if ( oldEntry && ( oldEntry->query().isNull() || !oldEntry->query()->numResults() || oldEntry->query()->results().first().data() != result.data() ) )
    {
        oldEntry->setIsPlaying( false );
    }
}


void
PlayableModel::onPlaybackStopped()
{
    Q_D( PlayableModel );
    PlayableItem* oldEntry = itemFromIndex( d->currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }
}


void
PlayableModel::ensureResolved( const QModelIndex& parent )
{
    QList< query_ptr > ql;
    for ( int i = 0; i < rowCount( parent ); i++ )
    {
        const QModelIndex idx = index( i, 0, parent );
        if ( hasChildren( idx ) )
            ensureResolved( idx );

        query_ptr query = itemFromIndex( idx )->query();
        if ( !query )
            continue;

        if ( !query->resolvingFinished() )
            ql << query;
    }

    Pipeline::instance()->resolve( ql );
}


Qt::Alignment
PlayableModel::columnAlignment( int column ) const
{
    switch ( column )
    {
        case Age:
        case AlbumPos:
        case Bitrate:
        case Duration:
        case Filesize:
        case Score:
        case Year:
            return Qt::AlignHCenter;
            break;

        default:
            return Qt::AlignLeft;
    }
}


QString
PlayableModel::scoreText( float score ) const
{
    static QMap<float, QString> texts;
    if ( texts.isEmpty() )
    {
        texts[ 1.0 ] = tr( "Perfect match" );
        texts[ 0.9 ] = tr( "Very good match" );
        texts[ 0.7 ] = tr( "Good match" );
        texts[ 0.5 ] = tr( "Vague match" );
        texts[ 0.3 ] = tr( "Bad match" );
        texts[ 0.1 ] = tr( "Very bad match" );
        texts[ 0.0 ] = tr( "Not available" );
        texts[ -1.0 ] = tr( "Searching..." );
    }

    if ( score == 1.0 )
        return texts[ 1.0 ];
    if ( score > 0.9 )
        return texts[ 0.9 ];
    if ( score > 0.7 )
        return texts[ 0.7 ];
    if ( score > 0.5 )
        return texts[ 0.5 ];
    if ( score > 0.3 )
        return texts[ 0.3 ];
    if ( score > 0.0 )
        return texts[ 0.1 ];
    if ( score == 0.0 )
        return texts[ 0.0 ];

    return texts[ -1.0 ];
}


void
PlayableModel::onDataChanged()
{
    PlayableItem* p = (PlayableItem*)sender();
    if ( p && p->index.isValid() )
        emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount() - 1 ) );
}


void
PlayableModel::startLoading()
{
    Q_D( PlayableModel );
    if ( !d->loading )
    {
        // tDebug() << Q_FUNC_INFO;
        d->loading = true;
        emit loadingStarted();
    }
}


void
PlayableModel::finishLoading()
{
    Q_D( PlayableModel );
    if ( d->loading )
    {
        tDebug() << Q_FUNC_INFO;
        d->loading = false;
        emit loadingFinished();
    }
}


PlayableItem*
PlayableModel::itemFromIndex( const QModelIndex& index ) const
{
    Q_D( const PlayableModel );
    if ( index.isValid() )
    {
        return static_cast<PlayableItem*>( index.internalPointer() );
    }
    else
    {
        return d->rootItem;
    }
}


void
PlayableModel::appendArtist( const Tomahawk::artist_ptr& artist )
{
    QList< artist_ptr > artists;
    artists << artist;

    appendArtists( artists );
}


void
PlayableModel::appendAlbum( const Tomahawk::album_ptr& album )
{
    QList< album_ptr > albums;
    albums << album;

    appendAlbums( albums );
}


void
PlayableModel::appendQuery( const Tomahawk::query_ptr& query )
{
    QList< query_ptr > queries;
    queries << query;

    appendQueries( queries );
}


void
PlayableModel::appendArtists( const QList< Tomahawk::artist_ptr >& artists )
{
    insertArtists( artists, rowCount( QModelIndex() ) );
}


void
PlayableModel::appendAlbums( const QList< Tomahawk::album_ptr >& albums )
{
    insertAlbums( albums, rowCount( QModelIndex() ) );
}


void
PlayableModel::appendAlbums( const Tomahawk::collection_ptr& collection )
{
    emit loadingStarted();
    insertAlbums( collection, rowCount( QModelIndex() ) );
}


void
PlayableModel::appendQueries( const QList< Tomahawk::query_ptr >& queries )
{
    insertQueries( queries, rowCount( QModelIndex() ) );
}


void
PlayableModel::appendTracks( const QList< Tomahawk::track_ptr >& tracks, const QList< Tomahawk::PlaybackLog >& logs )
{
    emit loadingStarted();
    QList< Tomahawk::query_ptr > queries;
    foreach ( const track_ptr& track, tracks )
    {
        queries << track->toQuery();
    }

    insertQueries( queries, rowCount( QModelIndex() ), logs );
}


void
PlayableModel::appendTracks( const Tomahawk::collection_ptr& collection )
{
    emit loadingStarted();
    insertTracks( collection, rowCount( QModelIndex() ) );
}


void
PlayableModel::insertArtist( const Tomahawk::artist_ptr& artist, int row )
{
    QList< artist_ptr > artists;
    artists << artist;

    insertArtists( artists, row );
}


void
PlayableModel::insertAlbum( const Tomahawk::album_ptr& album, int row )
{
    QList< album_ptr > albums;
    albums << album;

    insertAlbums( albums, row );
}


void
PlayableModel::insertQuery( const Tomahawk::query_ptr& query, int row, const Tomahawk::PlaybackLog& log, const QModelIndex& parent )
{
    QList< query_ptr > queries;
    queries << query;
    QList< Tomahawk::PlaybackLog > logs;
    logs << log;

    insertQueries( queries, row, logs, parent );
}


void
PlayableModel::insertArtists( const QList< Tomahawk::artist_ptr >& artists, int row )
{
    insertInternal( artists, row );
}


void
PlayableModel::insertAlbums( const QList< Tomahawk::album_ptr >& albums, int row )
{
    insertInternal( albums, row );
}


void
PlayableModel::insertAlbums( const Tomahawk::collection_ptr& collection, int /* row */ )
{
    Tomahawk::AlbumsRequest* req = collection->requestAlbums( Tomahawk::artist_ptr() );
    connect( dynamic_cast< QObject* >( req ), SIGNAL( albums( QList< Tomahawk::album_ptr > ) ),
             this, SLOT( appendAlbums( QList< Tomahawk::album_ptr > ) ), Qt::UniqueConnection );
    req->enqueue();
}


void
PlayableModel::insertQueries( const QList< Tomahawk::query_ptr >& queries, int row, const QList< Tomahawk::PlaybackLog >& logs, const QModelIndex& parent )
{
    insertInternal( queries, row, logs, parent );
}


void
PlayableModel::insertTracks( const Tomahawk::collection_ptr& collection, int /* row */ )
{
    Tomahawk::TracksRequest* req = collection->requestTracks( Tomahawk::album_ptr() );
    connect( dynamic_cast< QObject* >( req ), SIGNAL( tracks( QList< Tomahawk::query_ptr > ) ),
             this, SLOT( appendQueries( QList< Tomahawk::query_ptr > ) ), Qt::UniqueConnection );
    req->enqueue();

//    connect( collection.data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
}


void
PlayableModel::setTitle( const QString& title )
{
    Q_D( PlayableModel );
    d->title = title;
    emit changed();
}


QString
PlayableModel::description() const
{
    Q_D( const PlayableModel );
    return d->description;
}


void
PlayableModel::setDescription( const QString& description )
{
    Q_D( PlayableModel );
    d->description = description;
    emit changed();
}


QPixmap
PlayableModel::icon() const
{
    Q_D( const PlayableModel );
    return d->icon;
}


void
PlayableModel::setIcon( const QPixmap& pixmap )
{
    Q_D( PlayableModel );
    d->icon = pixmap;
    emit changed();
}


int
PlayableModel::trackCount() const
{
    return rowCount( QModelIndex() );
}


int
PlayableModel::itemCount() const
{
    return rowCount( QModelIndex() );
}


void
PlayableModel::onQueryBecamePlayable( bool playable )
{
    Q_UNUSED( playable );

    Tomahawk::Query* q = qobject_cast< Query* >( sender() );
    if ( !q )
    {
        // Track has been removed from the playlist by now
        return;
    }

    Tomahawk::query_ptr query = q->weakRef().toStrongRef();
    PlayableItem* item = itemFromQuery( query );
    if ( item )
    {
        emit indexPlayable( item->index );
    }
}


void
PlayableModel::onQueryResolved( bool hasResults )
{
    Q_UNUSED( hasResults );

    Tomahawk::Query* q = qobject_cast< Query* >( sender() );
    if ( !q )
    {
        // Track has been removed from the playlist by now
        return;
    }

    Tomahawk::query_ptr query = q->weakRef().toStrongRef();
    PlayableItem* item = itemFromQuery( query );
    if ( item )
    {
        emit indexResolved( item->index );
    }
}


PlayableItem*
PlayableModel::itemFromQuery( const Tomahawk::query_ptr& query, const QModelIndex& parent ) const
{
    if ( !query )
        return 0;

    for ( int i = 0; i < rowCount( parent ); i++ )
    {
        QModelIndex idx = index( i, 0, parent );
        if ( hasChildren( idx ) )
        {
            PlayableItem* subItem = itemFromQuery( query, idx );
            if ( subItem )
                return subItem;
        }

        PlayableItem* item = itemFromIndex( idx );
        if ( item && item->query() == query )
        {
            return item;
        }
    }

    if ( !parent.isValid() )
        tDebug() << Q_FUNC_INFO << "Could not find item for query in entire model:" << query->toString();

    return 0;
}


PlayableItem*
PlayableModel::itemFromResult( const Tomahawk::result_ptr& result, const QModelIndex& parent ) const
{
    if ( !result )
        return 0;

    for ( int i = 0; i < rowCount( parent ); i++ )
    {
        QModelIndex idx = index( i, 0, parent );
        if ( hasChildren( idx ) )
        {
            PlayableItem* subItem = itemFromResult( result, idx );
            if ( subItem )
                return subItem;
        }

        PlayableItem* item = itemFromIndex( idx );
        if ( item && item->result() == result )
        {
            return item;
        }
    }

    if ( !parent.isValid() )
        tDebug() << Q_FUNC_INFO << "Could not find item for result in entire model:" << result->toString();

    return 0;
}


QModelIndex
PlayableModel::indexFromSource( const Tomahawk::source_ptr& source ) const
{
    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        QModelIndex idx = index( i, 0, QModelIndex() );
        PlayableItem* item = itemFromIndex( idx );
        if ( item && item->source() == source )
        {
            return idx;
        }
    }

    // tDebug() << Q_FUNC_INFO << "Could not find item for source:" << source->friendlyName();
    return QModelIndex();
}
