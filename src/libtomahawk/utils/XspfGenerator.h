/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef XSPFGENERATOR_H
#define XSPFGENERATOR_H

#include "Typedefs.h"

#include <QtCore/qobject.h>


class XSPFGenerator : public QObject
{
    Q_OBJECT
public:
    explicit XSPFGenerator( const Tomahawk::playlist_ptr& pl, QObject* parent = 0 );
    virtual ~XSPFGenerator();

signals:
    void generated( const QByteArray& xspf );

private slots:
    void generate();

private:
    Tomahawk::playlist_ptr m_playlist;
};

#endif // XSPFGENERATOR_H
