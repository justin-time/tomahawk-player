/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "RecentPlaylistsModel.h"

#include "audio/AudioEngine.h"
#include "collection/Collection.h"
#include "database/Database.h"
#include "database/DatabaseCommand_LoadAllSortedPlaylists.h"
#include "network/Servent.h"
#include "playlist/dynamic/DynamicPlaylist.h"
#include "utils/Logger.h"

#include "PlaylistEntry.h"
#include "RecentlyPlayedPlaylistsModel.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "Track.h"

#define REFRESH_TIMEOUT 1000

using namespace Tomahawk;


RecentPlaylistsModel::RecentPlaylistsModel( unsigned int maxPlaylists, QObject* parent )
    : QAbstractListModel( parent )
    , m_maxPlaylists( maxPlaylists )
{
    m_timer = new QTimer( this );

    connect( m_timer, SIGNAL( timeout() ), SLOT( onRefresh() ) );
    connect( SourceList::instance(), SIGNAL( ready() ), SLOT( onReady() ) );

    // Load recent playlists initially
    if ( SourceList::instance()->isReady() )
        onRefresh();
}


void
RecentPlaylistsModel::refresh()
{
    if ( m_timer->isActive() )
        m_timer->stop();
    m_timer->start( REFRESH_TIMEOUT );
}


void
RecentPlaylistsModel::onRefresh()
{
    if ( m_timer->isActive() )
        m_timer->stop();

    emit loadingStarted();

    DatabaseCommand_LoadAllSortedPlaylists* cmd = new DatabaseCommand_LoadAllSortedPlaylists( source_ptr() );
    cmd->setLimit( m_maxPlaylists );
    cmd->setSortOrder( DatabaseCommand_LoadAllPlaylists::ModificationTime );
    cmd->setSortAscDesc( DatabaseCommand_LoadAllPlaylists::Descending );
    connect( cmd, SIGNAL( done( QList<Tomahawk::DatabaseCommand_LoadAllSortedPlaylists::SourcePlaylistPair> ) ),
             this, SLOT( playlistsLoaded( QList<Tomahawk::DatabaseCommand_LoadAllSortedPlaylists::SourcePlaylistPair> ) ) );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
RecentPlaylistsModel::onReady()
{
    foreach ( const source_ptr& s, SourceList::instance()->sources() )
        onSourceAdded( s );

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), this, SLOT( onSourceAdded( Tomahawk::source_ptr ) ), Qt::QueuedConnection );
    onRefresh();
}


void
RecentPlaylistsModel::playlistsLoaded( const QList<DatabaseCommand_LoadAllSortedPlaylists::SourcePlaylistPair>& playlistGuids )
{
    beginResetModel();
    m_playlists.clear();

    DatabaseCommand_LoadAllSortedPlaylists::SourcePlaylistPair plPair;
    foreach ( plPair, playlistGuids )
    {
        const playlist_ptr& pl = Playlist::get( plPair.second );
        if ( !pl )
        {
            tDebug() << "ERROR: Found a playlist that is not associated with any source:" << plPair.first << plPair.second;
            continue;
        }

        connect( pl.data(), SIGNAL( changed() ), SLOT( updatePlaylist() ) );
        m_playlists << pl;

        if ( !pl->loaded() )
            pl->loadRevision();
    }

    endResetModel();

    emit emptinessChanged( m_playlists.isEmpty() );
    emit loadingFinished();
}


QVariant
RecentPlaylistsModel::data( const QModelIndex& index, int role ) const
{
    if ( !index.isValid() || !hasIndex( index.row(), index.column(), index.parent() ) )
        return QVariant();

    playlist_ptr pl = m_playlists[index.row()];
    switch( role )
    {
    case Qt::DisplayRole:
        return pl->title();
    case RecentlyPlayedPlaylistsModel::PlaylistRole:
        return QVariant::fromValue< Tomahawk::playlist_ptr >( pl );
    case RecentlyPlayedPlaylistsModel::ArtistRole:
    {
        if ( m_artists.value( pl ).isEmpty() )
        {
            QStringList artists;

            foreach ( const Tomahawk::plentry_ptr& entry, pl->entries() )
            {
                if ( !artists.contains( entry->query()->track()->artist() ) )
                    artists << entry->query()->track()->artist();
            }

            m_artists[pl] = artists.join( ", " );
        }

        return m_artists[pl];
    }
    case RecentlyPlayedPlaylistsModel::PlaylistTypeRole:
    {
        if ( !pl.dynamicCast< Tomahawk::DynamicPlaylist >().isNull() )
        {
            dynplaylist_ptr dynp = pl.dynamicCast< Tomahawk::DynamicPlaylist >();
            if ( dynp->mode() == Static )
                return RecentlyPlayedPlaylistsModel::AutoPlaylist;
            else if ( dynp->mode() == OnDemand )
                return RecentlyPlayedPlaylistsModel::Station;
        }
        else
        {
            return RecentlyPlayedPlaylistsModel::StaticPlaylist;
        }
    }
    case RecentlyPlayedPlaylistsModel::DynamicPlaylistRole:
    {
        dynplaylist_ptr dynp = pl.dynamicCast< Tomahawk::DynamicPlaylist >();
        return QVariant::fromValue< Tomahawk::dynplaylist_ptr >( dynp );
    }
    case RecentlyPlayedPlaylistsModel::TrackCountRole:
    {
        if ( !pl.dynamicCast< Tomahawk::DynamicPlaylist >().isNull() && pl.dynamicCast< Tomahawk::DynamicPlaylist >()->mode() == OnDemand )
            return QString( QChar( 0x221E ) );
        else
            return pl->entries().count();
    }
    default:
        return QVariant();
    }
}


void
RecentPlaylistsModel::updatePlaylist()
{
    Playlist* p = qobject_cast< Playlist* >( sender() );
    Q_ASSERT( p );

    for ( int i = 0; i < m_playlists.size(); i++ )
    {
        if ( m_playlists[ i ].isNull() )
            continue;

        if ( m_playlists[ i ]->guid() == p->guid() )
        {
            QModelIndex idx = index( i, 0, QModelIndex() );
            emit dataChanged( idx, idx );
        }
    }
}


void
RecentPlaylistsModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source.data(), SIGNAL( online() ), this, SLOT( sourceOnline() ) );
    connect( source->dbCollection().data(), SIGNAL( playlistsAdded( QList<Tomahawk::playlist_ptr> ) ), SLOT( refresh() ), Qt::QueuedConnection );
    connect( source->dbCollection().data(), SIGNAL( autoPlaylistsAdded(QList<Tomahawk::dynplaylist_ptr>)), SLOT( refresh() ), Qt::QueuedConnection );
    connect( source->dbCollection().data(), SIGNAL( stationsAdded(QList<Tomahawk::dynplaylist_ptr>)), SLOT( refresh() ), Qt::QueuedConnection );
    connect( source->dbCollection().data(), SIGNAL( playlistsDeleted( QList<Tomahawk::playlist_ptr> ) ), SLOT( onPlaylistsRemoved( QList<Tomahawk::playlist_ptr> ) ) );
    connect( source->dbCollection().data(), SIGNAL( autoPlaylistsDeleted(QList<Tomahawk::dynplaylist_ptr>) ), SLOT( onDynPlaylistsRemoved( QList<Tomahawk::dynplaylist_ptr> ) ) );
    connect( source->dbCollection().data(), SIGNAL( stationsDeleted(QList<Tomahawk::dynplaylist_ptr>) ), SLOT( onDynPlaylistsRemoved( QList<Tomahawk::dynplaylist_ptr> ) ) );
}


void
RecentPlaylistsModel::sourceOnline()
{
    Source* s = qobject_cast< Source* >( sender() );
    Q_ASSERT( s );

    for ( int i = 0; i < m_playlists.size(); i++ )
    {
        if ( m_playlists[ i ]->author().data() == s )
        {
            QModelIndex idx = index( i, 0, QModelIndex() );
            emit dataChanged( idx, idx );
        }
    }
}


void
RecentPlaylistsModel::onDynPlaylistsRemoved( QList< dynplaylist_ptr > playlists )
{
    QList< playlist_ptr > pls;
    foreach ( const dynplaylist_ptr& p, playlists )
        pls << p;

    onPlaylistsRemoved( pls );
}


void
RecentPlaylistsModel::onPlaylistsRemoved( QList< playlist_ptr > playlists )
{
    foreach ( const playlist_ptr& pl, playlists )
    {
        if ( m_playlists.contains( pl ) )
        {
            m_artists.remove( pl );

            int idx = m_playlists.indexOf( pl );
            beginRemoveRows( QModelIndex(), idx, idx );
            m_playlists.removeAt( idx );
            endRemoveRows();
        }
    }

    emit emptinessChanged( m_playlists.isEmpty() );
}


int
RecentPlaylistsModel::rowCount( const QModelIndex& ) const
{
    return m_playlists.count();
}
