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

#include "DatabaseCommand_LogPlayback.h"

#include "collection/Collection.h"
#include "database/Database.h"
#include "network/Servent.h"
#include "utils/Logger.h"

#include "DatabaseImpl.h"
#include "PlaylistEntry.h"

#include <QDateTime>
#include <QSqlQuery>

#define STARTED_THRESHOLD 600   // Don't advertise tracks older than X seconds as currently playing
#define FINISHED_THRESHOLD 10   // Don't store tracks played less than X seconds in the playback log
#define SUBMISSION_THRESHOLD 20 // Don't broadcast playback logs when a track was played less than X seconds

using namespace Tomahawk;


void
DatabaseCommand_LogPlayback::postCommitHook()
{
    connect( this, SIGNAL( trackPlaying( Tomahawk::track_ptr, unsigned int ) ),
             source().data(), SLOT( onPlaybackStarted( Tomahawk::track_ptr, unsigned int ) ), Qt::QueuedConnection );
    connect( this, SIGNAL( trackPlayed( Tomahawk::track_ptr, Tomahawk::PlaybackLog ) ),
             source().data(), SLOT( onPlaybackFinished( Tomahawk::track_ptr, Tomahawk::PlaybackLog ) ), Qt::QueuedConnection );

    track_ptr track = Track::get( m_artist, m_track, QString() );
    if ( !track )
        return;

    if ( m_action == Finished )
    {
        PlaybackLog log;
        log.source = source();
        log.timestamp = m_playtime;
        log.secsPlayed = m_secsPlayed;

        emit trackPlayed( track, log );
    }
    // if the play time is more than 10 minutes in the past, ignore
    else if ( m_action == Started && QDateTime::fromTime_t( playtime() ).secsTo( QDateTime::currentDateTime() ) < STARTED_THRESHOLD )
    {
        emit trackPlaying( track, m_trackDuration );
    }

    if ( source()->isLocal() )
    {
        Servent::instance()->triggerDBSync();
    }
}


void
DatabaseCommand_LogPlayback::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( !source().isNull() );

    unsigned int pt = m_playtime;
    if ( m_playtime == 0 )
        m_playtime = QDateTime::currentDateTimeUtc().toTime_t();

    if ( m_action != Finished )
        return;
    if ( m_secsPlayed < FINISHED_THRESHOLD && m_trackDuration > 0 )
        return;
    if ( m_artist.isEmpty() || m_track.isEmpty() )
        return;

    QVariant srcid = source()->isLocal() ? QVariant( QVariant::Int ) : source()->id();
    TomahawkSqlQuery query = dbi->newquery();

    if ( pt > 0 && source()->isLocal() )
    {
        query.prepare( QString( "SELECT * FROM playback_log WHERE source %1 AND playtime = %2" ).arg( srcid.isNull() ? "IS NULL" : srcid.toString() ).arg( m_playtime ) );
        query.exec();
        if ( query.next() )
        {
            tDebug() << "Ignoring dupe playback log for source" << srcid << "with timestamp" << m_playtime;
            return;
        }
    }

//    tDebug() << "Logging playback of" << m_artist << "-" << m_track << "for source" << srcid << "- timestamp:" << m_playtime;

    query.prepare( "INSERT INTO playback_log(source, track, playtime, secs_played) VALUES (?, ?, ?, ?)" );
    query.bindValue( 0, srcid );

    // If there's no artist, because it's a resolver result with bad metadata for example, don't save it
    int artid = dbi->artistId( m_artist, true );
    if( artid < 1 )
        return;

    int trkid = dbi->trackId( artid, m_track, true );
    if( trkid < 1 )
        return;

    query.bindValue( 1, trkid );
    query.bindValue( 2, m_playtime );
    query.bindValue( 3, m_secsPlayed );

    query.exec();
}


bool
DatabaseCommand_LogPlayback::localOnly() const
{
    if ( m_action == Finished )
        return m_secsPlayed < SUBMISSION_THRESHOLD;

    return false;
}

