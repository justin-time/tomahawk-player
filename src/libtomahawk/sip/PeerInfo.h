/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Dominik Schmidt <dev@dominik-schmidt.de>
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

#ifndef PEERINFO_H
#define PEERINFO_H

#include "DllMacro.h"
#include "utils/TomahawkUtils.h"

#include <QString>
#include <QPixmap>

#define peerInfoDebug(peerInfo) tLog( LOGVERBOSE ) << "PEERINFO:" << ( !peerInfo.isNull() ? peerInfo->debugName() : "Invalid PeerInfo" ).toLatin1().constData()

class ControlConnection;
class SipPlugin;
class SipInfo;

namespace Tomahawk
{

class PeerInfoPrivate;

class DLLEXPORT PeerInfo : public QObject
{
Q_OBJECT

public:
    enum Status
    {
        Online,
        Offline
    };

    enum GetOptions
    {
        None,
        AutoCreate
    };

    // this is a uberstupid hack, identify characteristics of the type
    enum Type
    {
        External, // this is the default
        Local
    };

    static Tomahawk::peerinfo_ptr getSelf( SipPlugin* parent, GetOptions options = None );
    static QList< Tomahawk::peerinfo_ptr > getAllSelf();

    static Tomahawk::peerinfo_ptr get( SipPlugin* parent, const QString& id, GetOptions options = None );
    static QList< Tomahawk::peerinfo_ptr > getAll();

    virtual ~PeerInfo();

    const QString id() const;
    SipPlugin* sipPlugin() const;
    const QString debugName() const;
    void sendLocalSipInfos( const QList<SipInfo>& sipInfos );

    QWeakPointer< Tomahawk::PeerInfo > weakRef();
    void setWeakRef( QWeakPointer< Tomahawk::PeerInfo > weakRef );

    void setControlConnection( ControlConnection* controlConnection );
    ControlConnection* controlConnection() const;
    bool hasControlConnection();

    void setType( Tomahawk::PeerInfo::Type type );
    PeerInfo::Type type() const;

    /* actual data */

    // while peerId references a certain peer, contact id references the contact
    // e.g. a peerId might be a full jid with resource while contact id is the bare jid
    void setContactId( const QString& contactId );
    const QString contactId() const;

    void setStatus( Status status );
    Status status() const;

    void setSipInfos( const QList<SipInfo>& sipInfos );
    const QList<SipInfo> sipInfos() const;

    void setFriendlyName( const QString& friendlyName );
    const QString friendlyName() const;

    void setAvatar( const QPixmap& avatar );
    const QPixmap avatar( TomahawkUtils::ImageMode style = TomahawkUtils::Original, const QSize& size = QSize() ) const;

    void setVersionString( const QString& versionString );
    const QString versionString() const;

    // you can store arbitrary internal data for your plugin here
    void setData( const QVariant& data );
    const QVariant data() const;

    /**
     * Get the node id of this peer
     */
    const QString nodeId() const;

    /**
     * Get the authentication key for this host
     */
    const QString key() const;

signals:
    void sipInfoChanged();

private:
    PeerInfo( SipPlugin* parent, const QString& id );
    void announce();

    Q_DECLARE_PRIVATE( Tomahawk::PeerInfo )
    QScopedPointer< Tomahawk::PeerInfoPrivate > d_ptr;

    static QHash< SipPlugin*, peerinfo_ptr > s_selfPeersBySipPlugin;
};


} // ns


#endif // PEERINFO_H
