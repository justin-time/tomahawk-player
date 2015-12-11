/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2014,      Teo Mrnjavac <teo@kde.org>
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

#include "CollectionViewPage.h"

#include <QRadioButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "audio/AudioEngine.h"
#include "widgets/FilterHeader.h"
#include "playlist/TreeModel.h"
#include "playlist/ColumnView.h"
#include "playlist/TrackView.h"
#include "playlist/GridItemDelegate.h"
#include "playlist/GridView.h"
#include "playlist/PlayableProxyModelPlaylistInterface.h"
#include "resolvers/ScriptCollection.h"
#include "TomahawkSettings.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Closure.h"
#include "utils/Logger.h"

using namespace Tomahawk;


CollectionViewPage::CollectionViewPage( const Tomahawk::collection_ptr& collection, QWidget* parent )
    : QWidget( parent )
    , m_header( new FilterHeader( this ) )
    , m_columnView( new ColumnView() )
    , m_trackView( new TrackView() )
    , m_albumView( new GridView() )
    , m_model( 0 )
    , m_flatModel( 0 )
    , m_albumModel( 0 )
{
    qRegisterMetaType< CollectionViewPageMode >( "CollectionViewPageMode" );

    m_header->setBackground( ImageRegistry::instance()->pixmap( RESPATH "images/collection_background.png", QSize( 0, 0 ) ), false );
    setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultCollection, TomahawkUtils::Original, QSize( 256, 256 ) ) );

    m_columnView->proxyModel()->setStyle( PlayableProxyModel::Collection );

    m_trackView->setColumnHidden( PlayableModel::Composer, true );
    m_trackView->setColumnHidden( PlayableModel::Origin, true );
    m_trackView->setColumnHidden( PlayableModel::Score, true );
    m_trackView->setGuid( QString( "trackview/flat" ) );

    {
        m_albumView->setAutoResize( false );
        m_albumView->setAutoFitItems( true );
        m_albumView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        m_albumView->setItemWidth( TomahawkUtils::DpiScaler::scaledX( this, 170 ) );
        m_albumView->delegate()->setWordWrapping( true );

        m_albumView->setEmptyTip( tr( "Sorry, there are no albums in this collection!" ) );

        TomahawkStyle::stylePageFrame( m_albumView );

        m_albumView->setStyleSheet( QString( "QListView { background-color: %1; }" ).arg( TomahawkStyle::PAGE_BACKGROUND.name() ) );
    }

    m_stack = new QStackedWidget();
    setLayout( new QVBoxLayout() );
    TomahawkUtils::unmarginLayout( layout() );

    {
        m_header->ui->anchor1Label->setText( tr( "Artists" ) );
        m_header->ui->anchor2Label->setText( tr( "Albums" ) );
        m_header->ui->anchor3Label->setText( tr( "Songs" ) );
        m_header->ui->anchor1Label->show();
        m_header->ui->anchor2Label->show();
        m_header->ui->anchor3Label->show();

        const float lowOpacity = 0.8;
        m_header->ui->anchor1Label->setOpacity( 1 );
        m_header->ui->anchor2Label->setOpacity( lowOpacity );
        m_header->ui->anchor3Label->setOpacity( lowOpacity );

        QFontMetrics fm( m_header->ui->anchor1Label->font() );
        m_header->ui->anchor1Label->setFixedWidth( fm.width( m_header->ui->anchor1Label->text() ) + 16 );
        m_header->ui->anchor2Label->setFixedWidth( fm.width( m_header->ui->anchor2Label->text() ) + 16 );
        m_header->ui->anchor3Label->setFixedWidth( fm.width( m_header->ui->anchor3Label->text() ) + 16 );

        NewClosure( m_header->ui->anchor1Label, SIGNAL( clicked() ), const_cast< CollectionViewPage* >( this ), SLOT( setCurrentMode( CollectionViewPageMode ) ), CollectionViewPage::Columns )->setAutoDelete( false );
        NewClosure( m_header->ui->anchor2Label, SIGNAL( clicked() ), const_cast< CollectionViewPage* >( this ), SLOT( setCurrentMode( CollectionViewPageMode ) ), CollectionViewPage::Albums )->setAutoDelete( false );
        NewClosure( m_header->ui->anchor3Label, SIGNAL( clicked() ), const_cast< CollectionViewPage* >( this ), SLOT( setCurrentMode( CollectionViewPageMode ) ), CollectionViewPage::Flat )->setAutoDelete( false );
    }

    layout()->addWidget( m_header );
    layout()->addWidget( m_stack );

    m_stack->addWidget( m_columnView );
    m_stack->addWidget( m_albumView );
    m_stack->addWidget( m_trackView );

    connect( m_header, SIGNAL( filterTextChanged( QString ) ), SLOT( setFilter( QString ) ) );

    loadCollection( collection );
}


CollectionViewPage::~CollectionViewPage()
{
    tDebug() << Q_FUNC_INFO;
}


void
CollectionViewPage::loadCollection( const collection_ptr& collection )
{
    if ( m_collection )
    {
        disconnect( collection.data(), SIGNAL( changed() ), this, SLOT( onCollectionChanged() ) );
    }

    m_collection = collection;
    connect( collection.data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );

    onCollectionChanged();
}


void
CollectionViewPage::setTreeModel( TreeModel* model )
{
    QPointer<PlayableModel> oldModel = m_model;
    m_model = model;
    m_columnView->setTreeModel( model );

    connect( model, SIGNAL( changed() ), SLOT( onModelChanged() ), Qt::UniqueConnection );
    onModelChanged();

    if ( oldModel )
    {
        disconnect( oldModel.data(), SIGNAL( changed() ), this, SLOT( onModelChanged() ) );
        delete oldModel.data();
    }
}


void
CollectionViewPage::setFlatModel( PlayableModel* model )
{
    QPointer<PlayableModel> oldModel = m_flatModel;

    m_flatModel = model;
    m_trackView->setPlayableModel( model );
    m_trackView->setSortingEnabled( true );
    m_trackView->sortByColumn( 0, Qt::AscendingOrder );

    if ( oldModel )
    {
        disconnect( oldModel.data(), SIGNAL( changed() ), this, SLOT( onModelChanged() ) );
        delete oldModel.data();
    }
}


void
CollectionViewPage::setAlbumModel( PlayableModel* model )
{
    QPointer<PlayableModel> oldModel = m_albumModel;

    if ( m_albumModel )
        delete m_albumModel;

    m_albumModel = model;
    m_albumView->setPlayableModel( model );
    m_albumView->proxyModel()->sort( PlayableModel::Artist, Qt::AscendingOrder );

    if ( oldModel )
    {
        disconnect( oldModel.data(), SIGNAL( changed() ), this, SLOT( onModelChanged() ) );
        delete oldModel.data();
    }
}


void
CollectionViewPage::setCurrentMode( CollectionViewPageMode mode )
{
    if ( m_mode != mode )
    {
        TomahawkSettings::instance()->beginGroup( "ui" );
        TomahawkSettings::instance()->setValue( "flexibleTreeViewMode", mode );
        TomahawkSettings::instance()->endGroup();
        TomahawkSettings::instance()->sync();

        m_mode = mode;
    }

    QFont inactive = m_header->ui->anchor1Label->font();
    inactive.setBold( false );
    QFont active = m_header->ui->anchor1Label->font();
    active.setBold( true );
    const float lowOpacity = 0.8;

    switch ( mode )
    {
        case Albums:
        {
            m_header->ui->anchor2Label->setOpacity( 1 );
            m_header->ui->anchor1Label->setOpacity( lowOpacity );
            m_header->ui->anchor3Label->setOpacity( lowOpacity );

            m_header->ui->anchor2Label->setFont( active );
            m_header->ui->anchor1Label->setFont( inactive );
            m_header->ui->anchor3Label->setFont( inactive );

            m_stack->setCurrentWidget( m_albumView );
            break;
        }

        case Columns:
        {
            m_header->ui->anchor1Label->setOpacity( 1 );
            m_header->ui->anchor2Label->setOpacity( lowOpacity );
            m_header->ui->anchor3Label->setOpacity( lowOpacity );

            m_header->ui->anchor1Label->setFont( active );
            m_header->ui->anchor2Label->setFont( inactive );
            m_header->ui->anchor3Label->setFont( inactive );

            m_stack->setCurrentWidget( m_columnView );
            break;
        }

        case Flat:
        {
            m_header->ui->anchor3Label->setOpacity( 1 );
            m_header->ui->anchor1Label->setOpacity( lowOpacity );
            m_header->ui->anchor2Label->setOpacity( lowOpacity );

            m_header->ui->anchor3Label->setFont( active );
            m_header->ui->anchor1Label->setFont( inactive );
            m_header->ui->anchor2Label->setFont( inactive );

            m_stack->setCurrentWidget( m_trackView );
            break;
        }
    }

    emit modeChanged( mode );
}


Tomahawk::playlistinterface_ptr
CollectionViewPage::playlistInterface() const
{
    return m_columnView->proxyModel()->playlistInterface();
}


QString
CollectionViewPage::title() const
{
    return m_model->title();
}


QString
CollectionViewPage::description() const
{
    return m_model->description();
}


QPixmap
CollectionViewPage::pixmap() const
{
    return m_pixmap;
}


bool
CollectionViewPage::jumpToCurrentTrack()
{
    tDebug() << Q_FUNC_INFO;

    bool b = false;

    // note: the order of comparison is important here, if we'd write "b || foo" then foo will not be executed if b is already true!
    b = m_columnView->jumpToCurrentTrack() || b;
    b = m_trackView->jumpToCurrentTrack() || b;
    b = m_albumView->jumpToCurrentTrack() || b;

    return b;
}


bool
CollectionViewPage::setFilter( const QString& pattern )
{
    ViewPage::setFilter( pattern );

    m_columnView->setFilter( pattern );
    m_trackView->setFilter( pattern );
    m_albumView->setFilter( pattern );

    return true;
}


void
CollectionViewPage::restoreViewMode()
{
    //FIXME: needs be moved to TomahawkSettings
    TomahawkSettings::instance()->beginGroup( "ui" );
    int modeNumber = TomahawkSettings::instance()->value( "flexibleTreeViewMode", Columns ).toInt();
    m_mode = static_cast< CollectionViewPageMode >( modeNumber );
    TomahawkSettings::instance()->endGroup();

    setCurrentMode( (CollectionViewPageMode)modeNumber );
}


void
CollectionViewPage::setEmptyTip( const QString& tip )
{
    m_columnView->setEmptyTip( tip );
    m_albumView->setEmptyTip( tip );
    m_trackView->setEmptyTip( tip );
}


void
CollectionViewPage::setPixmap( const QPixmap& pixmap, bool tinted )
{
    m_pixmap = pixmap;
    m_header->setPixmap( pixmap, tinted );
}


void
CollectionViewPage::onModelChanged()
{
    setPixmap( m_model->icon(), false );
    m_header->setCaption( m_model->title() );
    m_header->setDescription( m_model->description() );
}


void
CollectionViewPage::onCollectionChanged()
{
    TreeModel* model = new TreeModel();
    PlayableModel* flatModel = new PlayableModel();
    PlayableModel* albumModel = new PlayableModel();

    setTreeModel( model );
    setFlatModel( flatModel );
    setAlbumModel( albumModel );

    model->addCollection( m_collection );
    flatModel->appendTracks( m_collection );
    albumModel->appendAlbums( m_collection );

    if ( m_collection && m_collection->source() && m_collection->source()->isLocal() )
    {
        setEmptyTip( tr( "After you have scanned your music collection you will find your tracks right here." ) );
    }
    else
        setEmptyTip( tr( "This collection is empty." ) );

    if ( m_collection.objectCast<ScriptCollection>() )
        m_trackView->setEmptyTip( tr( "Cloud collections aren't supported in the flat view yet. We will have them covered soon. Switch to another view to navigate them." ) );
}


bool
CollectionViewPage::isTemporaryPage() const
{
    return false;
}


bool
CollectionViewPage::isBeingPlayed() const
{
    if ( !playlistInterface() )
        return false;

    if ( playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
        return true;

    return false;
}
