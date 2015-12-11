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

#ifndef COLUMNITEMDELEGATE_H
#define COLUMNITEMDELEGATE_H

#include <QStyledItemDelegate>

#include "DllMacro.h"
#include "Typedefs.h"

namespace Tomahawk {
class PixmapDelegateFader;
}

class ColumnView;
class TreeProxyModel;

class DLLEXPORT ColumnItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    ColumnItemDelegate( ColumnView* parent, TreeProxyModel* proxy );

    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

public slots:
    void resetHoverIndex();

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

private slots:
    void doUpdateIndex( const QPersistentModelIndex& index );

private:
    ColumnView* m_view;
    TreeProxyModel* m_model;

    mutable QHash< QPersistentModelIndex, QSharedPointer< Tomahawk::PixmapDelegateFader > > m_pixmaps;
    mutable QHash< QPersistentModelIndex, QRect > m_infoButtonRects;
    QPersistentModelIndex m_hoveringOver;
};

#endif // COLUMNITEMDELEGATE_H
