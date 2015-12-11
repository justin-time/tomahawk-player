/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "Account.h"

#include "AccountManager.h"
#include "ConfigStorage.h"

namespace Tomahawk
{

namespace Accounts
{

QString
accountTypeToString( AccountType type )
{
    switch ( type )
    {
        case SipType:
            return QObject::tr( "Friend Finders" );
        case ResolverType:
            return QObject::tr( "Music Finders" );
        case InfoType:
        case StatusPushType:
            return QObject::tr( "Status Updaters" );
        case NoType:
            return QString();
    }

    return QString();
}


Account::Account( const QString& accountId )
    : QObject()
    , m_accountId( accountId )
{
    m_cfg.enabled = false;

    connect( this, SIGNAL( error( int, QString ) ), this, SLOT( onError( int, QString ) ) );
    connect( this, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) , this, SLOT( onConnectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) );

    loadFromConfig( accountId );
}


Account::~Account()
{
}


AccountConfigWidget*
Account::configurationWidget()
{
    return 0;
}


QWidget*
Account::aclWidget()
{
    return 0;
}


QPixmap
Account::icon() const
{
    return QPixmap();
}


void
Account::authenticate()
{
    return;
}


void
Account::deauthenticate()
{
    return;
}


bool
Account::isAuthenticated() const
{
    return false;
}


void
Account::onError( int errorCode, const QString& error )
{
    Q_UNUSED( errorCode );

    QMutexLocker locker( &m_mutex );
    m_cachedError = error;
}


void
Account::onConnectionStateChanged( Account::ConnectionState )
{
    m_cachedError.clear();
}


void
Account::syncConfig()
{
    AccountManager::instance()->configStorageForAccount( m_accountId )->save( m_accountId, m_cfg );
}


void
Account::loadFromConfig( const QString& accountId )
{
    m_accountId = accountId;

    if ( AccountManager::instance()->configStorageForAccount( m_accountId ) != 0 ) //could be 0 if we are installing the account right now
        AccountManager::instance()->configStorageForAccount( m_accountId )->load( m_accountId, m_cfg );
}


void
Account::removeFromConfig()
{
    AccountManager::instance()->configStorageForAccount( m_accountId )->remove( m_accountId );
}


void
Account::setTypes( AccountTypes types )
{
    QMutexLocker locker( &m_mutex );
    m_cfg.types = QStringList();
    if ( types & InfoType )
        m_cfg.types << "InfoType";
    if ( types & SipType )
        m_cfg.types << "SipType";
    if ( types & ResolverType )
        m_cfg.types << "ResolverType";
    if ( types & StatusPushType )
        m_cfg.types << "StatusPushType";
    syncConfig();
}


AccountTypes
Account::types() const
{
    QMutexLocker locker( &m_mutex );
    AccountTypes types;
    if ( m_cfg.types.contains( "InfoType" ) )
        types |= InfoType;
    if ( m_cfg.types.contains( "SipType" ) )
        types |= SipType;
    if ( m_cfg.types.contains( "ResolverType" ) )
        types |= ResolverType;
    if ( m_cfg.types.contains( "StatusPushType" ) )
        types |= StatusPushType;

    return types;
}


}

}
