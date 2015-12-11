/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ColumnItemDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QMouseEvent>

#include "Query.h"
#include "Result.h"

#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "utils/Closure.h"
#include "utils/PixmapDelegateFader.h"

#include "PlayableItem.h"
#include "TreeProxyModel.h"
#include "ColumnView.h"
#include "ViewManager.h"
#include "Typedefs.h"


ColumnItemDelegate::ColumnItemDelegate( ColumnView* parent, TreeProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
}


QSize
ColumnItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size;

    if ( index.isValid() )
    {
        Tomahawk::ModelTypes type = (Tomahawk::ModelTypes)index.data( PlayableProxyModel::TypeRole ).toInt();
        switch ( type )
        {
            case Tomahawk::TypeAlbum:
            {
                size.setHeight( option.fontMetrics.height() * 5 );
                return size;
            }

            case Tomahawk::TypeQuery:
            case Tomahawk::TypeResult:
            {
                size.setHeight( option.fontMetrics.height() * 1.6 );
                return size;
            }

            default:
                break;
        }
    }

    // artist per default
    size.setHeight( option.fontMetrics.height() * 3 );
    return size;
}


void
ColumnItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item )
        return;

    QTextOption textOption( Qt::AlignVCenter | (Qt::Alignment)index.data( Qt::TextAlignmentRole ).toUInt() );
    textOption.setWrapMode( QTextOption::NoWrap );

    QString text;
    if ( !item->artist().isNull() )
    {
        text = item->artist()->name();
    }
    else if ( !item->album().isNull() )
    {
        text = item->album()->name();
    }
    else if ( !item->result().isNull() || !item->query().isNull() )
    {
        float opacity = item->result() && item->result()->isOnline() ? item->result()->score() : 0.0;
        opacity = qMax( (float)0.3, opacity );
        QColor textColor = TomahawkUtils::alphaBlend( option.palette.color( QPalette::Foreground ), option.palette.color( QPalette::Background ), opacity );

        {
            QStyleOptionViewItemV4 o = option;
            initStyleOption( &o, QModelIndex() );

            painter->save();
            o.palette.setColor( QPalette::Text, textColor );

            if ( m_view->currentIndex() == index )
                o.state |= QStyle::State_Selected;
            else
                o.state &= ~QStyle::State_Selected;

            if ( o.state & QStyle::State_Selected && o.state & QStyle::State_Active )
            {
                o.palette.setColor( QPalette::Text, o.palette.color( QPalette::HighlightedText ) );
            }

            if ( item->isPlaying() )
            {
                textColor = TomahawkStyle::NOW_PLAYING_ITEM_TEXT;
                o.palette.setColor( QPalette::Highlight, TomahawkStyle::NOW_PLAYING_ITEM );
                o.palette.setColor( QPalette::Text, TomahawkStyle::NOW_PLAYING_ITEM_TEXT );
                o.state |= QStyle::State_Selected;
            }

            int oldX = 0;
//            if ( m_view->header()->visualIndex( index.column() ) == 0 )
            {
                oldX = o.rect.x();
                o.rect.setX( 0 );
            }
            qApp->style()->drawControl( QStyle::CE_ItemViewItem, &o, painter );
            if ( oldX > 0 )
                o.rect.setX( oldX );

/*            if ( m_hoveringOver == index && !index.data().toString().isEmpty() && index.column() == 0 )
            {
                o.rect.setWidth( o.rect.width() - o.rect.height() );
                QRect arrowRect( o.rect.x() + o.rect.width(), o.rect.y() + 1, o.rect.height() - 2, o.rect.height() - 2 );

                QPixmap infoIcon = TomahawkUtils::defaultPixmap( TomahawkUtils::InfoIcon, TomahawkUtils::Original, arrowRect.size() );
                painter->drawPixmap( arrowRect, infoIcon );

                m_infoButtonRects[ index ] = arrowRect;
            }*/

            {
                QRect r = o.rect.adjusted( 3, 0, 0, 0 );

                // Paint Now Playing Speaker Icon
                if ( item->isPlaying() )
                {
                    const int pixMargin = 1;
                    const int pixHeight = r.height() - pixMargin * 2;
                    QRect npr = r.adjusted( pixMargin, pixMargin, pixHeight - r.width() + pixMargin, -pixMargin );
                    painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
                    r.adjust( pixHeight + 6, 0, 0, 0 );
                }

                painter->setPen( o.palette.text().color() );

                QString text = index.data().toString();
                if ( item->query()->track()->albumpos() > 0 )
                {
                    text = QString( "%1. %2" )
                              .arg( index.data( PlayableModel::AlbumPosRole ).toString() )
                              .arg( index.data().toString() );
                }

                text = painter->fontMetrics().elidedText( text, Qt::ElideRight, r.width() - 3 );
                painter->drawText( r.adjusted( 0, 1, 0, 0 ), text, textOption );
            }
            painter->restore();
        }

        return;
    }
    else
        return;

    if ( text.trimmed().isEmpty() )
        text = tr( "Unknown" );

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, QModelIndex() );

    const QModelIndex curIndex = m_view->currentIndex();
    if ( curIndex == index || curIndex.parent() == index || curIndex.parent().parent() == index )
        opt.state |= QStyle::State_Selected;
    else
        opt.state &= ~QStyle::State_Selected;

    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( opt.state & QStyle::State_Selected )
    {
        opt.palette.setColor( QPalette::Text, opt.palette.color( QPalette::HighlightedText ) );
    }

    QRect arrowRect( m_view->viewport()->width() - option.rect.height(), option.rect.y() + 1, option.rect.height() - 2, option.rect.height() - 2 );
    if ( m_hoveringOver.row() == index.row() && m_hoveringOver.parent() == index.parent() )
    {
        QPixmap infoIcon = TomahawkUtils::defaultPixmap( TomahawkUtils::InfoIcon, TomahawkUtils::Original, arrowRect.size() );
        painter->drawPixmap( arrowRect, infoIcon );

        m_infoButtonRects[ index ] = arrowRect;
    }

    if ( index.column() > 0 )
        return;

    painter->save();
    painter->setRenderHint( QPainter::TextAntialiasing );
    painter->setPen( opt.palette.color( QPalette::Text ) );

    QRect r = option.rect.adjusted( 8, 2, -option.rect.width() + option.rect.height() + 4, -2 );
//    painter->drawPixmap( r, QPixmap( RESPATH "images/cover-shadow.png" ) );

    if ( !m_pixmaps.contains( index ) )
    {
        if ( !item->album().isNull() )
        {
            m_pixmaps.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->album(), r.size(), TomahawkUtils::Original, false ) ) );
            _detail::Closure* closure = NewClosure( m_pixmaps[ index ], SIGNAL( repaintRequest() ), const_cast<ColumnItemDelegate*>(this), SLOT( doUpdateIndex( const QPersistentModelIndex& ) ), QPersistentModelIndex( index ) );
            closure->setAutoDelete( false );
        }
        else if ( !item->artist().isNull() )
        {
            m_pixmaps.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->artist(), r.size(), TomahawkUtils::Original, false ) ) );
            _detail::Closure* closure = NewClosure( m_pixmaps[ index ], SIGNAL( repaintRequest() ), const_cast<ColumnItemDelegate*>(this), SLOT( doUpdateIndex( const QPersistentModelIndex& ) ), QPersistentModelIndex( index ) );
            closure->setAutoDelete( false );
        }
    }

    const QPixmap cover = m_pixmaps[ index ]->currentPixmap();
    painter->drawPixmap( r, cover );

    QFont font = painter->font();
//    font.setWeight( 58 );
    font.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
    painter->setFont( font );

    r = option.rect.adjusted( option.rect.height() + 16, 6, -4, -6 );
    text = painter->fontMetrics().elidedText( text, Qt::ElideRight, r.width() );
    painter->drawText( r, text, textOption );

    painter->restore();
}


void
ColumnItemDelegate::doUpdateIndex( const QPersistentModelIndex& index )
{
    PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item )
        return;

    item->forceUpdate();
}


bool
ColumnItemDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    Q_UNUSED( model );
    Q_UNUSED( option );

    if ( event->type() != QEvent::MouseButtonRelease &&
         event->type() != QEvent::MouseMove &&
         event->type() != QEvent::MouseButtonPress &&
         event->type() != QEvent::Leave )
        return false;

    bool hoveringInfo = false;
    if ( m_infoButtonRects.contains( index ) )
    {
        const QRect infoRect = m_infoButtonRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringInfo = infoRect.contains( ev->pos() );
    }

    if ( event->type() == QEvent::MouseMove )
    {
        if ( hoveringInfo )
            m_view->setCursor( Qt::PointingHandCursor );
        else
            m_view->setCursor( Qt::ArrowCursor );

        if ( m_hoveringOver != index )
        {
            PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
            item->requestRepaint();
            m_hoveringOver = index;
            doUpdateIndex( m_hoveringOver );
        }

        event->accept();
        return true;
    }

    // reset mouse cursor. we switch to a pointing hand cursor when hovering an info button
    m_view->setCursor( Qt::ArrowCursor );

    if ( hoveringInfo )
    {
        if ( event->type() == QEvent::MouseButtonRelease )
        {
            PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
            if ( !item )
                return false;

            if ( item->query() )
            {
                ViewManager::instance()->show( item->query()->track()->toQuery() );
            }
            else if ( item->artist() )
            {
                ViewManager::instance()->show( item->artist() );
            }
            else if ( item->album() )
            {
                ViewManager::instance()->show( item->album() );
            }

            event->accept();
            return true;
        }
        else if ( event->type() == QEvent::MouseButtonPress )
        {
            // Stop the whole item from having a down click action as we just want the info button to be clicked
            event->accept();
            return true;
        }
    }

    return false;
}


void
ColumnItemDelegate::resetHoverIndex()
{
    m_hoveringOver = QModelIndex();
    m_infoButtonRects.clear();
}
