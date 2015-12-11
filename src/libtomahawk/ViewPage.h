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

#ifndef VIEWPAGE_H
#define VIEWPAGE_H

#include "Typedefs.h"
#include "PlaylistInterface.h"
#include "Artist.h"
#include "Album.h"
#include "Source.h"
#include "utils/TomahawkUtils.h"
#include "playlist/PlaylistUpdaterInterface.h"

#include <QtGui/QPixmap>

namespace Tomahawk
{


class DLLEXPORT ViewPage
{
public:
    enum DescriptionType {
        TextType = 0,
        ArtistType = 1,
        AlbumType = 2
    };

    ViewPage() {}
    virtual ~ViewPage();

    virtual QWidget* widget() = 0;
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const = 0;

    virtual QString title() const = 0;

    virtual DescriptionType descriptionType() { return TextType; }
    virtual QString description() const = 0;
    virtual Tomahawk::artist_ptr descriptionArtist() const { return Tomahawk::artist_ptr(); }
    virtual Tomahawk::album_ptr descriptionAlbum() const { return Tomahawk::album_ptr(); }

    virtual QString longDescription() const { return QString(); }
    virtual QPixmap pixmap() const { return QPixmap( RESPATH "icons/tomahawk-icon-128x128.png" ); }

    virtual bool queueVisible() const { return true; }

    virtual QString filter() const { return m_filter; }
    virtual bool setFilter( const QString& filter );

    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual bool dropMimeData( const QMimeData*, Qt::DropAction );

    virtual bool jumpToCurrentTrack() = 0;

    virtual bool isTemporaryPage() const { return false; }

    /**
     * Should we add a row in the SourceTreeView for this page.
     */
    virtual bool addPageItem() const;

    /**
     * This page is actually a constant page that will be shown on every
     * restart of Tomahawk until the user selects it to be removed.
     *
     * The main distinction between this and isTemporaryPage() is that the
     * page will not be listed in the search history.
     */
    virtual bool isDeletable() const { return false; }

    /**
     * The ViewPage item in the SourcesModel was deleted.
     */
    virtual void onItemDeleted();

    virtual bool isBeingPlayed() const { return false; }

    virtual QList<PlaylistUpdaterInterface*> updaters() const { return QList<PlaylistUpdaterInterface*>(); }

    /** subclasses implementing ViewPage can emit the following signals:
     * nameChanged( const QString& )
     * descriptionChanged( const QString& )
     * descriptionChanged( const Tomahawk::artist_ptr& artist )
     * descriptionChanged( const Tomahawk::album_ptr& album )
     * longDescriptionChanged( const QString& )
     * pixmapChanged( const QPixmap& )
     * destroyed( QWidget* widget );
     *
     * See DynamicWidget for an example
     */

private:
    QString m_filter;
};

} // ns

Q_DECLARE_METATYPE( Tomahawk::ViewPage* )

#endif //VIEWPAGE_H
