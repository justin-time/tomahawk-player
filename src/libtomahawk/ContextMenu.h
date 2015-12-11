/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QSignalMapper>
#include <QMenu>

#include "Typedefs.h"
#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT ContextMenu : public QMenu
{
Q_OBJECT

public:
    enum MenuActions
    {
        ActionPlay =         1,
        ActionQueue =        2,
        ActionDelete =       4,
        ActionCopyLink =     8,
        ActionLove =         16,
        ActionStopAfter =    32,
        ActionPage =         64,
        ActionTrackPage =    65,
        ActionArtistPage =   66,
        ActionAlbumPage =    67,
        ActionEditMetadata = 128,
        ActionPlaylist =     256,
        ActionSend =         512,
        ActionMarkListened = 1024
    };

    explicit ContextMenu( QWidget* parent = 0 );
    virtual ~ContextMenu();

    int supportedActions() const { return m_supportedActions; }
    void setSupportedActions( int actions ) { m_supportedActions = actions; }

    void setPlaylistInterface( const Tomahawk::playlistinterface_ptr& plInterface );

    void setQuery( const Tomahawk::query_ptr& query );
    void setQueries( const QList<Tomahawk::query_ptr>& queries );

    void setArtist( const Tomahawk::artist_ptr& artist );
    void setArtists( const QList<Tomahawk::artist_ptr>& artists );

    void setAlbum( const Tomahawk::album_ptr& album );
    void setAlbums( const QList<Tomahawk::album_ptr>& albums );

    void clear();

    unsigned int itemCount() const;

signals:
    void triggered( int action );

private slots:
    void onTriggered( int action );
    void copyLink();
    void openPage( MenuActions action );
    void addToQueue();
    void addToPlaylist( int playlistIdx );
    void sendToSource( int sourceIdx );

    void onSocialActionsLoaded();

private:
    QSignalMapper* m_sigmap;
    QSignalMapper* m_playlists_sigmap;
    QSignalMapper* m_sources_sigmap;
    int m_supportedActions;

    QAction* m_loveAction;

    QList< Tomahawk::playlist_ptr > m_playlists;
    QList< Tomahawk::query_ptr > m_queries;
    QList< Tomahawk::artist_ptr > m_artists;
    QList< Tomahawk::album_ptr > m_albums;
    QList< Tomahawk::source_ptr > m_sources;

    Tomahawk::playlistinterface_ptr m_interface;
};

}; // ns

#endif
