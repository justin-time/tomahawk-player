/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef DATABASECOMMAND_H
#define DATABASECOMMAND_H

#include <QMetaType>
#include <QVariant>

#include "DllMacro.h"
#include "Typedefs.h"

namespace Tomahawk
{

class DatabaseCommandPrivate;
class DatabaseImpl;

class DLLEXPORT DatabaseCommand : public QObject
{
Q_OBJECT
Q_PROPERTY( QString guid READ guid WRITE setGuid )

public:
    enum State {
        PENDING = 0,
        RUNNING = 1,
        FINISHED = 2
    };

    explicit DatabaseCommand( QObject* parent = 0 );
    explicit DatabaseCommand( const Tomahawk::source_ptr& src, QObject* parent = 0 );

    DatabaseCommand( const DatabaseCommand &other ); //needed for QMetaType

    virtual ~DatabaseCommand();

    virtual QString commandname() const { return "DatabaseCommand"; }
    virtual bool doesMutates() const { return true; }
    State state() const;

    // if i make this pure virtual, i get compile errors in qmetatype.h.
    // we need Q_DECLARE_METATYPE to use in queued sig/slot connections.
    virtual void exec( DatabaseImpl* /*lib*/ ) { Q_ASSERT( false ); }

    void _exec( DatabaseImpl* lib );

    // stuff to do once transaction applied ok.
    // Don't change the database from in here, duh.
    void postCommit() { postCommitHook(); emitCommitted(); }
    virtual void postCommitHook(){}

    void setSource( const Tomahawk::source_ptr& s );
    const Tomahawk::source_ptr& source() const;

    virtual bool loggable() const { return false; }
    virtual bool groupable() const { return false; }
    virtual bool singletonCmd() const { return false; }
    virtual bool localOnly() const { return false; }

    virtual QVariant data() const;
    virtual void setData( const QVariant& data );

    QString guid() const;
    void setGuid( const QString& g );

    void emitFinished();
    void emitCommitted();
    void emitRunning();

    QWeakPointer< Tomahawk::DatabaseCommand > weakRef() const;
    void setWeakRef( QWeakPointer< Tomahawk::DatabaseCommand > weakRef );

signals:
    void running();
    void running( const Tomahawk::dbcmd_ptr& );

    void finished();
    void finished( const Tomahawk::dbcmd_ptr& );

    void committed();
    void committed( const Tomahawk::dbcmd_ptr& );
protected:
    explicit DatabaseCommand( QObject* parent, DatabaseCommandPrivate* d );

    QScopedPointer<DatabaseCommandPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE( DatabaseCommand )
};

}

Q_DECLARE_METATYPE( Tomahawk::DatabaseCommand )

#endif // DATABASECOMMAND_H
