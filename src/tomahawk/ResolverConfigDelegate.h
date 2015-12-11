/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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


#ifndef RESOLVERCONFIGDELEGATE_H
#define RESOLVERCONFIGDELEGATE_H

#include "ConfigDelegateBase.h"

class ResolverConfigDelegate : public ConfigDelegateBase
{
    Q_OBJECT
public:
    explicit ResolverConfigDelegate( QObject* parent = 0 );
    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual QRect checkRectForIndex( const QStyleOptionViewItem &option, const QModelIndex &idx, int role ) const;
    virtual QRect configRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx ) const;

private slots:
    void onConfigPressed ( const QModelIndex& );

signals:
    void openConfig( const QString& resolverPath );
};

#endif // RESOLVERCONFIGDELEGATE_H
