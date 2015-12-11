/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "RemoteCollection.h"

#include "utils/Logger.h"

using namespace Tomahawk;


RemoteCollection::RemoteCollection( source_ptr source, QObject* parent )
    :  DatabaseCollection( source, parent )
{
}


QString
RemoteCollection::prettyName() const
{
    return tr( "Collection of %1" ).arg( source()->friendlyName() );
}


// adding/removing is done by dbsyncconnection, and the dbcmd objects that modify
// the database will make us emit the appropriate signals (tracksAdded etc.)
void
RemoteCollection::addTracks( const QList<QVariant>& newitems )
{
    Q_UNUSED( newitems );
    Q_ASSERT( false );
}


void
RemoteCollection::removeTracks( const QDir& dir )
{
    Q_UNUSED( dir );
    Q_ASSERT( false );
}
