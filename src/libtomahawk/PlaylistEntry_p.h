/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef PLAYLISTENTRY_P_H
#define PLAYLISTENTRY_P_H

#include "PlaylistEntry.h"

namespace Tomahawk
{

class PlaylistEntryPrivate
{
public:
    PlaylistEntryPrivate( PlaylistEntry* q )
        : q_ptr( q )
    {
    }

    Q_DECLARE_PUBLIC( PlaylistEntry )
    PlaylistEntry* q_ptr;
private:
    QString guid;
    Tomahawk::query_ptr query;
    QString annotation;
    unsigned int duration;
    unsigned int lastmodified;
    source_ptr   lastsource;
    QString      resulthint;
};

} // Tomahawk

#endif // PLAYLISTENTRY_P_H
