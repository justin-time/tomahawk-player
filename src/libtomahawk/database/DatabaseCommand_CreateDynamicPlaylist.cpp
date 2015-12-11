/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "DatabaseCommand_CreateDynamicPlaylist.h"

#include "playlist/dynamic/DynamicPlaylist.h"
#include "playlist/dynamic/DynamicControl.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "network/Servent.h"
#include "utils/Json.h"
#include "utils/Logger.h"

#include "DatabaseImpl.h"
#include "PlaylistEntry.h"
#include "SourceList.h"
#include "TomahawkSqlQuery.h"

#include <QSqlQuery>
#include <QSqlDriver>

using namespace Tomahawk;


DatabaseCommand_CreateDynamicPlaylist::DatabaseCommand_CreateDynamicPlaylist( QObject* parent )
    : DatabaseCommand_CreatePlaylist( parent )
    , m_autoLoad( true )
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "creating dynamiccreatecommand 1";
}


DatabaseCommand_CreateDynamicPlaylist::DatabaseCommand_CreateDynamicPlaylist( const source_ptr& author,
                                                                const dynplaylist_ptr& playlist, bool autoLoad )
    : DatabaseCommand_CreatePlaylist( author, playlist.staticCast<Playlist>() )
    , m_playlist( playlist )
    , m_autoLoad( autoLoad )
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "creating dynamiccreatecommand 2";
}

DatabaseCommand_CreateDynamicPlaylist::~DatabaseCommand_CreateDynamicPlaylist()
{}

QVariant
DatabaseCommand_CreateDynamicPlaylist::playlistV() const
{
        if( m_v.isNull() )
            return TomahawkUtils::qobject2qvariant( (QObject*)m_playlist.data() );
        else
            return m_v;
}

void
DatabaseCommand_CreateDynamicPlaylist::exec( DatabaseImpl* lib )
{
    Q_ASSERT( !( m_playlist.isNull() && m_v.isNull() ) );
    Q_ASSERT( !source().isNull() );

    DatabaseCommand_CreatePlaylist::createPlaylist( lib, true );
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Created normal playlist, now creating additional dynamic info!";

    tLog( LOGVERBOSE ) << Q_FUNC_INFO <<  "Create dynamic execing!" << m_playlist << m_v;
    TomahawkSqlQuery cre = lib->newquery();

    cre.prepare( "INSERT INTO dynamic_playlist( guid, pltype, plmode, autoload ) "
                 "VALUES( ?, ?, ?, ? )" );

    if( m_playlist.isNull() ) {
        QVariantMap m = m_v.toMap();
        cre.addBindValue( m.value( "guid" ) );
        cre.addBindValue( m.value( "type" ) );
        cre.addBindValue( m.value( "mode" ) );
    } else {
        cre.addBindValue( m_playlist->guid() );
        cre.addBindValue( m_playlist->type() );
        cre.addBindValue( m_playlist->mode() );
    }
    cre.addBindValue( m_autoLoad ? "true" : "false" );
    cre.exec();
}


void
DatabaseCommand_CreateDynamicPlaylist::postCommitHook()
{
    if ( source().isNull() || source()->dbCollection().isNull() )
    {
        tDebug() << "Source has gone offline, not emitting to GUI.";
        return;
    }

    if(  !DatabaseCommand_CreatePlaylist::report() || report() == false )
        return;

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "..reporting..";
    if( m_playlist.isNull() ) {
        QMetaObject::invokeMethod( SourceList::instance(),
                                   "createDynamicPlaylist",
                                   Qt::BlockingQueuedConnection,
                                   QGenericArgument( "Tomahawk::source_ptr", (const void*)&source() ),
                                   Q_ARG( QVariant, m_v ) );
    }
    else
    {
        m_playlist->reportCreated( m_playlist );
    }
    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
