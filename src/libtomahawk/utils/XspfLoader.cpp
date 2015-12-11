/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011-2012, Leo Franchi            <lfranchi@kde.org>
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

#include "XspfLoader.h"

#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "playlist/XspfUpdater.h"
#include "utils/NetworkReply.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/NetworkAccessManager.h"

#include "Pipeline.h"
#include "Playlist.h"
#include "SourceList.h"
#include "Track.h"

#include <QDomDocument>

using namespace Tomahawk;


QString
XSPFLoader::errorToString( XSPFErrorCode error )
{
    switch ( error )
    {
    case ParseError:
        return tr( "Failed to parse contents of XSPF playlist" );
    case InvalidTrackError:
        return tr( "Some playlist entries were found without artist and track name, they will be omitted");
    case FetchError:
        return tr( "Failed to fetch the desired playlist from the network, or the desired file does not exist" );
    default:
        return QString();
    }
}


XSPFLoader::XSPFLoader( bool autoCreate, bool autoUpdate, QObject* parent, const QString& guid )
    : QObject( parent )
    , m_autoCreate( autoCreate )
    , m_autoUpdate( autoUpdate )
    , m_autoResolve( true )
    , m_autoDelete( true )
    , m_guid( guid )
    , m_NS( "http://xspf.org/ns/0/" )
{
    qRegisterMetaType< XSPFErrorCode >("XSPFErrorCode");

    if ( m_guid.isEmpty() )
    {
        m_guid = uuid();
    }
}


XSPFLoader::~XSPFLoader()
{
}


void
XSPFLoader::setOverrideTitle( const QString& newTitle )
{
    m_overrideTitle = newTitle;
}


void
XSPFLoader::setAutoResolveTracks( bool autoResolve )
{
    m_autoResolve = autoResolve;
}


void
XSPFLoader::setAutoDelete( bool autoDelete )
{
    m_autoDelete = autoDelete;
}


void
XSPFLoader::setErrorTitle( const QString& error)
{
   m_errorTitle = error;
}


QList< Tomahawk::query_ptr >
XSPFLoader::entries() const
{
    return m_entries;
}


QString
XSPFLoader::title() const
{
    return m_title;
}

playlist_ptr XSPFLoader::getPlaylistForRecentUrl()
{
    m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                   m_guid,
                                   m_title,
                                   m_info,
                                   m_creator,
                                   false,
                                   m_entries );

    // 10 minute default---for now, no way to change it
    new Tomahawk::XspfUpdater( m_playlist, 600000, m_autoUpdate, m_url.toString() );

    return m_playlist;
}


void
XSPFLoader::load( const QUrl& url )
{
    m_url = url;
    QNetworkRequest request( url );

    Q_ASSERT( Tomahawk::Utils::nam() != 0 );
    NetworkReply* reply = new NetworkReply( Tomahawk::Utils::nam()->get( request ) );

    connect( reply, SIGNAL( finished() ), SLOT( networkLoadFinished() ) );
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ), SLOT( networkError( QNetworkReply::NetworkError ) ) );
}


void
XSPFLoader::load( QFile& file )
{
    if ( file.open( QFile::ReadOnly ) )
    {
        m_body = file.readAll();
        gotBody();
    }
    else
    {
        reportError();
    }
}


void
XSPFLoader::reportError()
{
    emit error( FetchError );
    const QString errorMsg = errorToString( FetchError );
    if ( !m_errorTitle.isEmpty() )
        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( QString( "%1: %2" ).arg( m_errorTitle ).arg( errorMsg ) ) );
    else
        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( errorMsg ) );
    deleteLater();
}


void
XSPFLoader::networkLoadFinished()
{
    NetworkReply* r = qobject_cast<NetworkReply*>( sender() );
    r->deleteLater();

    if ( r->reply()->error() == QNetworkReply::NoError )
    {
        m_body = r->reply()->readAll();
        gotBody();
    }
}


void
XSPFLoader::networkError( QNetworkReply::NetworkError /* error */ )
{
    reportError();

    NetworkReply* r = qobject_cast<NetworkReply*>( sender() );
    r->deleteLater();
}


void
XSPFLoader::gotBody()
{
    QDomDocument xmldoc;
    bool namespaceProcessing = true;
    xmldoc.setContent( m_body, namespaceProcessing );
    QDomElement docElement( xmldoc.documentElement() );

    QString origTitle;
    QDomNodeList tracklist;
    QDomElement n = docElement.firstChildElement();
    for ( ; !n.isNull(); n = n.nextSiblingElement() )
    {
        if ( n.namespaceURI() == m_NS && n.localName() == "title" )
        {
            origTitle = n.text();
        }
        else if ( n.namespaceURI() == m_NS && n.localName() == "creator" )
        {
            m_creator = n.text();
        }
        else if ( n.namespaceURI() == m_NS && n.localName() == "info" )
        {
            m_info = n.text();
        }
        else if ( n.namespaceURI() == m_NS && n.localName() == "trackList" )
        {
            tracklist = n.childNodes();
        }
    }

    m_title = origTitle;
    if ( m_title.isEmpty() )
        m_title = tr( "New Playlist" );
    if ( !m_overrideTitle.isEmpty() )
        m_title = m_overrideTitle;

    bool shownError = false;
    for ( int i = 0; i < (int)tracklist.length(); i++ )
    {
        QDomNode e = tracklist.at( i );

        QString artist, album, track, duration, annotation, url;
        QDomElement n = e.firstChildElement();
        for ( ; !n.isNull(); n = n.nextSiblingElement() )
        {
            if ( n.namespaceURI() == m_NS && n.localName() == "duration" )
            {
                duration = n.text();
            }
            else if ( n.namespaceURI() == m_NS && n.localName() == "annotation" )
            {
                annotation = n.text();
            }
            else if ( n.namespaceURI() == m_NS && n.localName() == "creator" )
            {
                artist = n.text();
            }
            else if ( n.namespaceURI() == m_NS && n.localName() == "album" )
            {
                album = n.text();
            }
            else if ( n.namespaceURI() == m_NS && n.localName() == "title" )
            {
                track = n.text();
            }
            else if ( n.namespaceURI() == m_NS && n.localName() == "url" )
            {
                if ( !n.text().startsWith( "http" ) || TomahawkUtils::whitelistedHttpResultHint( n.text() ) )
                    url = n.text();
            }
            else if ( n.namespaceURI() == m_NS && n.localName() == "location" )
            {
                if ( !n.text().startsWith( "http" ) || TomahawkUtils::whitelistedHttpResultHint( n.text() ) )
                    url = n.text();
            }
        }

        if ( artist.isEmpty() || track.isEmpty() )
        {
            if ( !shownError )
            {
                emit error( InvalidTrackError );
                shownError = true;
            }
            continue;
        }

        track_ptr t = Tomahawk::Track::get( artist, track, album, QString(), duration.toInt() / 1000 );
        query_ptr q = Tomahawk::Query::get( t );
        if ( q.isNull() )
            continue;

        if ( !url.isEmpty() )
        {
            q->setResultHint( url );
            q->setSaveHTTPResultHint( true );
        }

        m_entries << q;
    }

    if ( m_autoResolve )
    {
        for ( int i = m_entries.size() - 1; i >= 0; i-- )
            Pipeline::instance()->resolve( m_entries[ i ] );
    }

    if ( origTitle.isEmpty() && m_entries.isEmpty() )
    {
        emit error( ParseError );
        if ( m_autoCreate )
            deleteLater();
        return;
    }

    if ( m_autoCreate )
    {
        emit ok( getPlaylistForRecentUrl() );
    }
    else
    {
        if ( !m_entries.isEmpty() )
            emit tracks( m_entries );
    }

    if ( m_autoDelete )
        deleteLater();
}
