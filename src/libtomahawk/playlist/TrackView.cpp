/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "TrackView.h"

#include "ViewHeader.h"
#include "PlayableModel.h"
#include "PlayableProxyModel.h"
#include "PlayableItem.h"
#include "DropJob.h"
#include "Source.h"
#include "TomahawkSettings.h"
#include "audio/AudioEngine.h"
#include "widgets/OverlayWidget.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Closure.h"
#include "utils/AnimatedSpinner.h"
#include "utils/Logger.h"
#include "InboxModel.h"

// Forward Declarations breaking QSharedPointer
#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    #include "collection/Collection.h"
    #include "utils/PixmapDelegateFader.h"
#endif

#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QDrag>

#define SCROLL_TIMEOUT 280

using namespace Tomahawk;

TrackView::TrackView( QWidget* parent )
    : QTreeView( parent )
    , m_model( 0 )
    , m_proxyModel( 0 )
    , m_delegate( 0 )
    , m_header( new ViewHeader( this ) )
    , m_overlay( new OverlayWidget( this ) )
    , m_loadingSpinner( new LoadingSpinner( this ) )
    , m_resizing( false )
    , m_dragging( false )
    , m_alternatingRowColors( false )
    , m_autoExpanding( true )
    , m_contextMenu( new ContextMenu( this ) )
{
    setFrameShape( QFrame::NoFrame );
    setAttribute( Qt::WA_MacShowFocusRect, 0 );

    setContentsMargins( 0, 0, 0, 0 );
    setMouseTracking( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropMode( QAbstractItemView::InternalMove );
    setDragDropOverwriteMode( false );
    setAllColumnsShowFocus( true );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setRootIsDecorated( false );
    setUniformRowHeights( true );
    setAlternatingRowColors( m_alternatingRowColors );
    setAutoResize( false );

    setHeader( m_header );
    setSortingEnabled( true );
    sortByColumn( -1 );
    setContextMenuPolicy( Qt::CustomContextMenu );

    m_timer.setInterval( SCROLL_TIMEOUT );

    // enable those connects if you want to enable lazily loading extra information for visible items
//    connect( verticalScrollBar(), SIGNAL( rangeChanged( int, int ) ), SLOT( onViewChanged() ) );
//    connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ), SLOT( onViewChanged() ) );
//    connect( &m_timer, SIGNAL( timeout() ), SLOT( onScrollTimeout() ) );

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );
    connect( m_contextMenu, SIGNAL( triggered( int ) ), SLOT( onMenuTriggered( int ) ) );

    setProxyModel( new PlayableProxyModel( this ) );
}


TrackView::~TrackView()
{
    tDebug() << Q_FUNC_INFO << ( m_guid.isEmpty() ? QString( "with empty guid" ) : QString( "with guid %1" ).arg( m_guid ) );

    if ( !m_guid.isEmpty() && proxyModel()->playlistInterface() )
    {
        tDebug() << Q_FUNC_INFO << "Storing shuffle & random mode settings for guid" << m_guid;

        TomahawkSettings* s = TomahawkSettings::instance();
        s->setShuffleState( m_guid, proxyModel()->playlistInterface()->shuffled() );
        s->setRepeatMode( m_guid, proxyModel()->playlistInterface()->repeatMode() );
    }
}


QString
TrackView::guid() const
{
    if ( m_guid.isEmpty() )
        return QString();

    return QString( "%1/%2" ).arg( m_guid ).arg( m_proxyModel->columnCount() );
}


void
TrackView::setGuid( const QString& newguid )
{
    if ( newguid == m_guid )
        return;

    if ( !newguid.isEmpty() )
    {
        tDebug() << Q_FUNC_INFO << "Setting guid on header" << newguid << "for a view with" << m_proxyModel->columnCount() << "columns";

        m_guid = newguid;
        m_header->setGuid( guid() );

        if ( !m_guid.isEmpty() && proxyModel()->playlistInterface() )
        {
            tDebug() << Q_FUNC_INFO << "Restoring shuffle & random mode settings for guid" << m_guid;

            TomahawkSettings* s = TomahawkSettings::instance();
            proxyModel()->playlistInterface()->setShuffled( s->shuffleState( m_guid ) );
            proxyModel()->playlistInterface()->setRepeatMode( s->repeatMode( m_guid ) );
        }
    }
}


void
TrackView::setProxyModel( PlayableProxyModel* model )
{
    if ( m_proxyModel )
    {
        disconnect( m_proxyModel, SIGNAL( rowsAboutToBeInserted( QModelIndex, int, int ) ), this, SLOT( onModelFilling() ) );
        disconnect( m_proxyModel, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), this, SLOT( onModelEmptyCheck() ) );
        disconnect( m_proxyModel, SIGNAL( filterChanged( QString ) ), this, SLOT( onFilterChanged( QString ) ) );
        disconnect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), this, SLOT( onViewChanged() ) );
        disconnect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), this, SLOT( verifySize() ) );
        disconnect( m_proxyModel, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), this, SLOT( verifySize() ) );
        disconnect( m_proxyModel, SIGNAL( expandRequest( QPersistentModelIndex ) ), this, SLOT( expand( QPersistentModelIndex ) ) );
        disconnect( m_proxyModel, SIGNAL( selectRequest( QPersistentModelIndex ) ), this, SLOT( select( QPersistentModelIndex ) ) );
        disconnect( m_proxyModel, SIGNAL( currentIndexChanged( QModelIndex, QModelIndex ) ), this, SLOT( onCurrentIndexChanged( QModelIndex, QModelIndex ) ) );
    }

    m_proxyModel = model;

    connect( m_proxyModel, SIGNAL( rowsAboutToBeInserted( QModelIndex, int, int ) ), SLOT( onModelFilling() ) );
    connect( m_proxyModel, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), SLOT( onModelEmptyCheck() ) );
    connect( m_proxyModel, SIGNAL( filterChanged( QString ) ), SLOT( onFilterChanged( QString ) ) );
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onViewChanged() ) );
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( verifySize() ) );
    connect( m_proxyModel, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), SLOT( verifySize() ) );
    connect( m_proxyModel, SIGNAL( expandRequest( QPersistentModelIndex ) ), SLOT( expand( QPersistentModelIndex ) ) );
    connect( m_proxyModel, SIGNAL( selectRequest( QPersistentModelIndex ) ), SLOT( select( QPersistentModelIndex ) ) );
    connect( m_proxyModel, SIGNAL( currentIndexChanged( QModelIndex, QModelIndex ) ), SLOT( onCurrentIndexChanged( QModelIndex, QModelIndex ) ) );

    m_delegate = new PlaylistItemDelegate( this, m_proxyModel );
    QTreeView::setItemDelegate( m_delegate );
    QTreeView::setModel( m_proxyModel );
}


void
TrackView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    tDebug() << "Explicitly use setPlayableModel instead";
    Q_ASSERT( false );
}


void
TrackView::setPlaylistItemDelegate( PlaylistItemDelegate* delegate )
{
    if ( m_delegate )
        delete m_delegate;

    m_delegate = delegate;
    QTreeView::setItemDelegate( delegate );

    verifySize();
}


void
TrackView::setPlayableModel( PlayableModel* model )
{
    if ( m_model )
    {
        disconnect( m_model, SIGNAL( loadingStarted() ), m_loadingSpinner, SLOT( fadeIn() ) );
        disconnect( m_model, SIGNAL( loadingFinished() ), m_loadingSpinner, SLOT( fadeOut() ) );
        disconnect( m_model, SIGNAL( changed() ), this, SIGNAL( modelChanged() ) );
    }

    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourcePlayableModel( m_model );
    }

    setAcceptDrops( true );
    m_header->setDefaultColumnWeights( m_proxyModel->columnWeights() );
    setGuid( m_proxyModel->guid() );

    switch( m_proxyModel->style() )
    {
        case PlayableProxyModel::Fancy:
            setHeaderHidden( true );
            setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        break;

        default:
            setHeaderHidden( false );
            setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    }

    connect( m_model, SIGNAL( loadingStarted() ), m_loadingSpinner, SLOT( fadeIn() ) );
    connect( m_model, SIGNAL( loadingFinished() ), m_loadingSpinner, SLOT( fadeOut() ) );
    connect( m_model, SIGNAL( changed() ), SIGNAL( modelChanged() ) );

    if ( m_model->isLoading() )
        m_loadingSpinner->fadeIn();

    if ( m_autoExpanding )
    {
        expandAll();
        selectFirstTrack();
    }

    onViewChanged();
    emit modelChanged();
}


void
TrackView::setEmptyTip( const QString& tip )
{
    m_emptyTip = tip;
    m_overlay->setText( tip );
}


void
TrackView::onModelFilling()
{
    QTreeView::setAlternatingRowColors( m_alternatingRowColors );
}


void
TrackView::onModelEmptyCheck()
{
    if ( !m_proxyModel->rowCount( QModelIndex() ) )
        QTreeView::setAlternatingRowColors( false );
}


void
TrackView::onCurrentIndexChanged( const QModelIndex& newIndex, const QModelIndex& oldIndex )
{
    if ( selectedIndexes().count() == 1 && currentIndex() == oldIndex )
    {
        selectionModel()->select( newIndex, QItemSelectionModel::SelectCurrent );
        currentChanged( newIndex, oldIndex );
        setCurrentIndex( newIndex );
    }
}


void
TrackView::onViewChanged()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    m_timer.start();
}


void
TrackView::onScrollTimeout()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    QModelIndex left = indexAt( viewport()->rect().topLeft() );
    while ( left.isValid() && left.parent().isValid() )
        left = left.parent();

    QModelIndex right = indexAt( viewport()->rect().bottomLeft() );
    while ( right.isValid() && right.parent().isValid() )
        right = right.parent();

    int max = m_proxyModel->playlistInterface()->trackCount();
    if ( right.isValid() )
        max = right.row();

    if ( !max )
        return;

    //FIXME
    for ( int i = left.row(); i <= max; i++ )
    {
        m_proxyModel->updateDetailedInfo( m_proxyModel->index( i, 0 ) );
    }
}


void
TrackView::startPlayingFromStart()
{
    if ( m_proxyModel->rowCount() == 0 )
        return;

    const QModelIndex index = m_proxyModel->index( 0, 0 );
    startAutoPlay( index );
}


void
TrackView::autoPlayResolveFinished( const query_ptr& query, int row )
{
    Q_ASSERT( !query.isNull() );
    Q_ASSERT( row >= 0 );

    if ( query.isNull() || row < 0  || query != m_autoPlaying )
        return;

    const QModelIndex index = m_proxyModel->index( row, 0 );
    if ( query->playable() )
    {
        onItemActivated( index );
        return;
    }

    // Try the next one..
    const QModelIndex sib = index.sibling( index.row() + 1, index.column() );
    if ( sib.isValid() )
        startAutoPlay( sib );
}


void
TrackView::currentChanged( const QModelIndex& current, const QModelIndex& previous )
{
    QTreeView::currentChanged( current, previous );

    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( current ) );
    if ( item && item->query() )
    {
        emit querySelected( item->query() );
    }
    else
    {
        emit querySelected( query_ptr() );
    }
}


void
TrackView::onItemActivated( const QModelIndex& index )
{
    if ( !index.isValid() )
        return;

    tryToPlayItem( index );
    emit itemActivated( index );
}


void
TrackView::startAutoPlay( const QModelIndex& index )
{
    if ( tryToPlayItem( index ) )
        return;

    // item isn't playable but still resolving
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item && !item->query().isNull() && !item->query()->resolvingFinished() )
    {
        m_autoPlaying = item->query(); // So we can kill it if user starts autoplaying this playlist again
        NewClosure( item->query().data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( autoPlayResolveFinished( Tomahawk::query_ptr, int ) ),
                    item->query(), index.row() );
        return;
    }

    // not playable at all, try next
    const QModelIndex sib = index.sibling( index.row() + 1, index.column() );
    if ( sib.isValid() )
        startAutoPlay( sib );
}


bool
TrackView::tryToPlayItem( const QModelIndex& index )
{
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item && !item->query().isNull() )
    {
        m_model->setCurrentIndex( m_proxyModel->mapToSource( index ) );
        AudioEngine::instance()->playItem( playlistInterface(), item->query() );

        return true;
    }

    return false;
}


void
TrackView::keyPressEvent( QKeyEvent* event )
{
    QTreeView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return )
    {
        onItemActivated( currentIndex() );
    }
    if ( event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace )
    {
        tDebug() << "Removing selected items from playlist";
        deleteSelectedItems();
    }
}


void
TrackView::onItemResized( const QModelIndex& index )
{
    m_delegate->updateRowSize( index );
}


void
TrackView::playItem()
{
    onItemActivated( m_contextMenuIndex );
}


void
TrackView::resizeEvent( QResizeEvent* event )
{
    QTreeView::resizeEvent( event );

    int sortSection = m_header->sortIndicatorSection();
    Qt::SortOrder sortOrder = m_header->sortIndicatorOrder();

    if ( m_header->checkState() && sortSection >= 0 )
    {
        // restoreState keeps overwriting our previous sort-order
        sortByColumn( sortSection, sortOrder );
    }

    if ( !model() )
        return;

    if ( model()->columnCount() == 1 )
    {
        m_header->resizeSection( 0, event->size().width() );
    }
}


bool
TrackView::eventFilter( QObject* obj, QEvent* event )
{
    if ( event->type() == QEvent::DragEnter )
    {
        QDragEnterEvent* e = static_cast<QDragEnterEvent*>(event);
        dragEnterEvent( e );
        return true;
    }
    if ( event->type() == QEvent::DragMove )
    {
        QDragMoveEvent* e = static_cast<QDragMoveEvent*>(event);
        dragMoveEvent( e );
        return true;
    }
    if ( event->type() == QEvent::DragLeave )
    {
        QDragLeaveEvent* e = static_cast<QDragLeaveEvent*>(event);
        dragLeaveEvent( e );
        return true;
    }
    if ( event->type() == QEvent::Drop )
    {
        QDropEvent* e = static_cast<QDropEvent*>(event);
        dropEvent( e );
        return true;
    }

    return QObject::eventFilter( obj, event );
}


void
TrackView::dragEnterEvent( QDragEnterEvent* event )
{
    tDebug() << Q_FUNC_INFO;
    QTreeView::dragEnterEvent( event );

    if ( !model() || model()->isReadOnly() || model()->isLoading() )
    {
        event->ignore();
        return;
    }

    if ( DropJob::acceptsMimeData( event->mimeData() ) )
    {
        m_dragging = true;
        m_dropRect = QRect();

        event->acceptProposedAction();
    }
}


void
TrackView::dragMoveEvent( QDragMoveEvent* event )
{
    QTreeView::dragMoveEvent( event );

    if ( !model() || model()->isReadOnly() || model()->isLoading() )
    {
        event->ignore();
        return;
    }

    if ( DropJob::acceptsMimeData( event->mimeData() ) )
    {
        setDirtyRegion( m_dropRect );
        const QPoint pos = event->pos();
        QModelIndex index = indexAt( pos );
        bool pastLast = false;

        if ( !index.isValid() && proxyModel()->rowCount( QModelIndex() ) > 0 )
        {
            index = proxyModel()->index( proxyModel()->rowCount( QModelIndex() ) - 1, 0, QModelIndex() );
            pastLast = true;
        }

        if ( index.isValid() )
        {
            const QRect rect = visualRect( index );
            m_dropRect = rect;

            // indicate that the item will be inserted above the current place
            const int gap = 5; // FIXME constant
            int yHeight = ( pastLast ? rect.bottom() : rect.top() ) - gap / 2;
            m_dropRect = QRect( 0, yHeight, width(), gap );

            event->acceptProposedAction();
        }

        setDirtyRegion( m_dropRect );
    }
}


void
TrackView::dragLeaveEvent( QDragLeaveEvent* event )
{
    QTreeView::dragLeaveEvent( event );

    m_dragging = false;
    setDirtyRegion( m_dropRect );
}


void
TrackView::dropEvent( QDropEvent* event )
{
    tDebug() << Q_FUNC_INFO;
    QTreeView::dropEvent( event );

    if ( event->isAccepted() )
    {
        tDebug() << "Ignoring accepted event!";
    }
    else if ( event->source() != this )
    {
        // This code shouldn't be required when the PlayableModel properly accepts the incoming drop.
        // If we remove it, the queue for some reason doesn't accept the drops yet, though.
        if ( DropJob::acceptsMimeData( event->mimeData() ) )
        {
            const QPoint pos = event->pos();
            const QModelIndex index = indexAt( pos );

            if ( !model()->isReadOnly() && !model()->isLoading() )
            {
                tDebug() << Q_FUNC_INFO << "Drop Event accepted at row:" << index.row();
                event->acceptProposedAction();
                model()->dropMimeData( event->mimeData(), event->proposedAction(), index.row(), 0, index.parent() );
            }
        }
    }

    m_dragging = false;
}


void
TrackView::leaveEvent( QEvent* event )
{
    QTreeView::leaveEvent( event );

    m_delegate->resetHoverIndex();
}


void
TrackView::paintEvent( QPaintEvent* event )
{
    QTreeView::paintEvent( event );
    QPainter painter( viewport() );

    if ( m_dragging )
    {
        // draw drop indicator
        {
            // draw indicator for inserting items
            QBrush blendedBrush = viewOptions().palette.brush( QPalette::Normal, QPalette::Highlight );
            QColor color = blendedBrush.color();

            const int y = ( m_dropRect.top() + m_dropRect.bottom() ) / 2;
            const int thickness = m_dropRect.height() / 2;

            int alpha = 255;
            const int alphaDec = alpha / ( thickness + 1 );
            for ( int i = 0; i < thickness; i++ )
            {
                color.setAlpha( alpha );
                alpha -= alphaDec;
                painter.setPen( color );
                painter.drawLine( 0, y - i, width(), y - i );
                painter.drawLine( 0, y + i, width(), y + i );
            }
        }
    }
}


void
TrackView::wheelEvent( QWheelEvent* event )
{
    QTreeView::wheelEvent( event );

    m_delegate->resetHoverIndex();
    repaint();
}


void
TrackView::onFilterChanged( const QString& )
{
    if ( selectedIndexes().count() )
        scrollTo( selectedIndexes().at( 0 ), QAbstractItemView::PositionAtCenter );

    if ( !filter().isEmpty() && !proxyModel()->playlistInterface()->trackCount() && model()->trackCount() )
    {
        m_overlay->setText( tr( "Sorry, your filter '%1' did not match any results." ).arg( filter() ) );
        m_overlay->show();
    }
    else
    {
        if ( model()->trackCount() )
        {
            m_overlay->hide();
        }
        else
        {
            m_overlay->setText( m_emptyTip );
            m_overlay->show();
        }
    }
}


void
TrackView::startDrag( Qt::DropActions supportedActions )
{
    QList<QPersistentModelIndex> pindexes;
    QModelIndexList indexes;
    foreach( const QModelIndex& idx, selectedIndexes() )
    {
        if ( ( m_proxyModel->flags( idx ) & Qt::ItemIsDragEnabled ) )
        {
            indexes << idx;
            pindexes << idx;
        }
    }

    if ( indexes.count() == 0 )
        return;

    tDebug() << "Dragging" << indexes.count() << "indexes";
    QMimeData* data = m_proxyModel->mimeData( indexes );
    if ( !data )
        return;

    QDrag* drag = new QDrag( this );
    drag->setMimeData( data );
    const QPixmap p = TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeTrack, indexes.count() );
    drag->setPixmap( p );
    drag->setHotSpot( QPoint( -20, -20 ) );

    Qt::DropAction action = drag->exec( supportedActions, Qt::CopyAction );
    if ( action == Qt::MoveAction )
    {
        m_proxyModel->removeIndexes( pindexes );
    }

    // delete drag; FIXME? On OSX it doesn't seem to get deleted automatically.
}


void
TrackView::onCustomContextMenu( const QPoint& pos )
{
    m_contextMenu->clear();
    m_contextMenu->setPlaylistInterface( playlistInterface() );

    QModelIndex idx = indexAt( pos );
    idx = idx.sibling( idx.row(), 0 );
    setContextMenuIndex( idx );

    if ( !idx.isValid() )
        return;

    if ( model() && !model()->isReadOnly() )
        m_contextMenu->setSupportedActions( m_contextMenu->supportedActions() | ContextMenu::ActionDelete );
    if ( model() && qobject_cast< InboxModel* >( model() ) )
        m_contextMenu->setSupportedActions( m_contextMenu->supportedActions() | ContextMenu::ActionMarkListened
                                                                              | ContextMenu::ActionDelete );

    QList<query_ptr> queries;
    foreach ( const QModelIndex& index, selectedIndexes() )
    {
        if ( index.column() )
            continue;

        PlayableItem* item = proxyModel()->itemFromIndex( proxyModel()->mapToSource( index ) );
        if ( item && !item->query().isNull() )
        {
            queries << item->query();
        }
    }

    m_contextMenu->setQueries( queries );
    m_contextMenu->exec( viewport()->mapToGlobal( pos ) );
}


void
TrackView::onMenuTriggered( int action )
{
    switch ( action )
    {
        case ContextMenu::ActionPlay:
            onItemActivated( m_contextMenuIndex );
            break;

        case ContextMenu::ActionDelete:
            deleteSelectedItems();
            break;

        default:
            break;
    }
}


Tomahawk::playlistinterface_ptr
TrackView::playlistInterface() const
{
    return proxyModel()->playlistInterface();
}


void
TrackView::setPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlistInterface )
{
    proxyModel()->setPlaylistInterface( playlistInterface );
}


QString
TrackView::title() const
{
    return model()->title();
}


QString
TrackView::description() const
{
    return model()->description();
}


QPixmap
TrackView::pixmap() const
{
    return model()->icon();
}


bool
TrackView::jumpToCurrentTrack()
{
    scrollTo( proxyModel()->currentIndex(), QAbstractItemView::PositionAtCenter );
    selectionModel()->select( QModelIndex(), QItemSelectionModel::SelectCurrent );
    select( proxyModel()->currentIndex() );
    selectionModel()->select( proxyModel()->currentIndex(), QItemSelectionModel::SelectCurrent );
    return true;
}


bool
TrackView::setFilter( const QString& filter )
{
    ViewPage::setFilter( filter );
    m_proxyModel->setFilter( filter );
    return true;
}


void
TrackView::deleteSelectedItems()
{
    if ( !model()->isReadOnly() )
    {
        proxyModel()->removeIndexes( selectedIndexes() );
    }
    else
    {
        tDebug() << Q_FUNC_INFO << "Error: Model is read-only!";
    }
}


void
TrackView::verifySize()
{
    if ( !autoResize() || !m_proxyModel || !m_proxyModel->rowCount() )
        return;

    unsigned int height = 0;
    for ( int i = 0; i < m_proxyModel->rowCount(); i++ )
    {
        height += indexRowSizeHint( m_proxyModel->index( i, 0 ) );
    }

    setFixedHeight( height + contentsMargins().top() + contentsMargins().bottom() );
}


void
TrackView::setAutoResize( bool b )
{
    m_autoResize = b;

    if ( m_autoResize )
        setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
}


void
TrackView::setAlternatingRowColors( bool enable )
{
    m_alternatingRowColors = enable;
    QTreeView::setAlternatingRowColors( enable );
}


void
TrackView::expand( const QPersistentModelIndex& idx )
{
    QTreeView::expand( idx );
}


void
TrackView::select( const QPersistentModelIndex& idx )
{
    if ( selectedIndexes().count() )
        return;

//    selectionModel()->select( idx, QItemSelectionModel::SelectCurrent );
    currentChanged( idx, QModelIndex() );
}


void
TrackView::selectFirstTrack()
{
    if ( !m_proxyModel->rowCount() )
        return;
    if ( selectedIndexes().count() )
        return;

    QModelIndex idx = m_proxyModel->index( 0, 0, QModelIndex() );
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( idx ) );
    if ( item->source() )
    {
        idx = m_proxyModel->index( 0, 0, idx );
        item = m_model->itemFromIndex( m_proxyModel->mapToSource( idx ) );
    }

    if ( item->query() )
    {
//        selectionModel()->select( idx, QItemSelectionModel::SelectCurrent );
        currentChanged( idx, QModelIndex() );
    }
}


PlayableModel*
TrackView::model() const
{
    return m_model.data();
}
