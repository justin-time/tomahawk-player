/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef DBSYNCCONNECTION_H
#define DBSYNCCONNECTION_H

#include "network/Connection.h"
#include "network/DBSyncConnectionState.h"
#include "database/Op.h"
#include "Typedefs.h"

#include <QObject>
#include <QTimer>
#include <QSharedPointer>
#include <QIODevice>



class DatabaseCommand;

class DBSyncConnection : public Connection
{
Q_OBJECT

public:
    explicit DBSyncConnection( Servent* s, const Tomahawk::source_ptr& src );
    virtual ~DBSyncConnection();

    void setup();
    Connection* clone();

signals:
    void stateChanged( Tomahawk::DBSyncConnectionState newstate, Tomahawk::DBSyncConnectionState oldstate, const QString& info );

protected slots:
    void handleMsg( msg_ptr msg ) Q_DECL_OVERRIDE;

public slots:
    void sendOps();
    /// trigger a re-sync to pick up any new ops
    void trigger();

private slots:
    void gotThem( const QVariantMap& m );

    void fetchOpsData( const QString& sinceguid );
    void sendOpsData( QString sinceguid, QString lastguid, QList< dbop_ptr > ops );
    void lastOpApplied();

    void check();

private:
    void synced();
    void changeState( Tomahawk::DBSyncConnectionState newstate );

    int m_fetchCount;
    Tomahawk::source_ptr m_source;
    QVariantMap m_uscache;

    QString m_lastSentOp;

    Tomahawk::DBSyncConnectionState m_state;
};

#endif // DBSYNCCONNECTION_H
