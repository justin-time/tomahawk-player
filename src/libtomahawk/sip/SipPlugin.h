/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *             2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *             2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef SIPPLUGIN_H
#define SIPPLUGIN_H

#include "Typedefs.h"

#include <QObject>
#include <QMenu>
#include <QString>

#include "DllMacro.h"

class SipInfo;
namespace Tomahawk
{
    namespace Accounts
    {
        class Account;
    }
    class PeerInfo;
}

class DLLEXPORT SipPlugin : public QObject
{
    Q_OBJECT

friend class Tomahawk::PeerInfo;

public:
    enum AddContactOptions { NoOptions = 0, SendInvite = 1 };

    SipPlugin();
    explicit SipPlugin( Tomahawk::Accounts::Account* account, QObject* parent = 0 );
    virtual ~SipPlugin();

    // plugin id is "pluginfactoryname_someuniqueid".  get it from SipPluginFactory::generateId
    QString pluginId() const;

    virtual bool isValid() const = 0;
    virtual const QString friendlyName() const;
    virtual const QString serviceName() const;
    virtual QString inviteString() const;
    virtual QMenu* menu();
    virtual Tomahawk::Accounts::Account* account() const;

    // peer infos
    virtual const QList< Tomahawk::peerinfo_ptr > peersOnline() const;

public slots:
    virtual void connectPlugin() = 0;
    virtual void disconnectPlugin() = 0;
    virtual void checkSettings() = 0;
    virtual void configurationChanged() = 0;

    virtual bool addContact( const QString& peerId, AddContactOptions options = NoOptions, const QString& msg = QString() ) = 0;

    /**
     * Send a list of SipInfos to all contacts.
     */
    virtual void sendSipInfos( const Tomahawk::peerinfo_ptr& receiver, const QList<SipInfo>& infos ) = 0;

signals:
    void peerStatusChanged( const Tomahawk::peerinfo_ptr& );
    void dataError( bool );
    void inviteSentSuccess( const QString& inviteId );
    void inviteSentFailure( const QString& inviteId );

    // new data for own source
    void avatarReceived ( const QPixmap& avatar );

    void addMenu( QMenu* menu );
    void removeMenu( QMenu* menu );

protected:
    void setAllPeersOffline();

    Tomahawk::Accounts::Account *m_account;
};

#endif
