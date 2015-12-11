/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "AccountManager.h"

#include "sip/SipPlugin.h"
#include "sip/SipStatusMessage.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "utils/Closure.h"
#include "utils/Logger.h"
#include "utils/PluginLoader.h"

#include "CredentialsManager.h"
#include "config.h"
#include "ResolverAccount.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "LocalConfigStorage.h"

#include <QCoreApplication>
#include <QSet>
#include <QTimer>


namespace Tomahawk
{

namespace Accounts
{


AccountManager* AccountManager::s_instance = 0;


AccountManager*
AccountManager::instance()
{
    return s_instance;
}


AccountManager::AccountManager( QObject *parent )
    : QObject( parent )
    , m_readyForSip( false )
    , m_completelyReady( false )
{
    s_instance = this;

    QTimer::singleShot( 0, this, SLOT( init() ) );
}


AccountManager::~AccountManager()
{
    disconnectAll();
    qDeleteAll( m_accounts );
    qDeleteAll( m_accountFactories );
}


void
AccountManager::init()
{
    if ( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().isNull() )
    {
        //We need the info system worker to be alive so that we can move info plugins into its thread
        QTimer::singleShot( 0, this, SLOT( init() ) );
        return;
    }

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( onSettingsChanged() ) );

    loadPluginFactories();

    // We include the resolver factory manually, not in a plugin
    ResolverAccountFactory* f = new ResolverAccountFactory();
    m_accountFactories[ f->factoryId() ] = f;
    registerAccountFactoryForFilesystem( f );

    emit readyForFactories(); //Notifies TomahawkApp to load the remaining AccountFactories, then Accounts from config
}


void
AccountManager::loadPluginFactories()
{
    QHash< QString, QObject* > plugins = Tomahawk::Utils::PluginLoader( "account" ).loadPlugins();
    foreach ( QObject* plugin, plugins.values() )
    {
        AccountFactory* accountfactory = qobject_cast<AccountFactory*>( plugin );
        if ( accountfactory )
        {
            tDebug() << Q_FUNC_INFO << "Loaded plugin factory:" << plugins.key( plugin ) << accountfactory->factoryId() << accountfactory->prettyName();
            m_accountFactories[ accountfactory->factoryId() ] = accountfactory;
        } else
        {
            tDebug() << Q_FUNC_INFO << "Loaded invalid plugin.." << plugins.key( plugin );
        }
    }
}


bool
AccountManager::hasPluginWithFactory( const QString& factory ) const
{
    foreach ( Account* account, m_accounts )
    {
        if ( factoryFromId( account->accountId() ) == factory )
            return true;
    }
    return false;

}


QString
AccountManager::factoryFromId( const QString& accountId ) const
{
    return accountId.split( "_" ).first();
}


AccountFactory*
AccountManager::factoryForAccount( Account* account ) const
{
    const QString factoryId = factoryFromId( account->accountId() );
    return m_accountFactories.value( factoryId, 0 );
}


void
AccountManager::enableAccount( Account* account )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( account->enabled() )
        return;

    account->authenticate();

    if ( account->preventEnabling() )
        return;

    account->setEnabled( true );
    m_enabledAccounts << account;

    account->sync();
}


void
AccountManager::disableAccount( Account* account )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( !account->enabled() )
        return;

    account->deauthenticate();
    account->setEnabled( false );
    m_enabledAccounts.removeAll( account );

    account->sync();
}


void
AccountManager::connectAll()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    foreach ( Account* acc, m_accounts )
    {
        if ( acc->enabled() )
        {
            if ( acc->sipPlugin() )
            {
                tDebug() << Q_FUNC_INFO << "Connecting" << acc->accountFriendlyName();
                acc->authenticate();

                m_enabledAccounts << acc;
            }
        }
    }

    m_connected = true;
}


void
AccountManager::disconnectAll()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    foreach ( Account* acc, m_enabledAccounts )
    {
        if ( acc->sipPlugin( false ) )
        {
            tDebug() << Q_FUNC_INFO << "Disconnecting" << acc->accountFriendlyName();
            acc->deauthenticate();

            m_enabledAccounts.removeAll( acc );
        }
    }

    SourceList::instance()->removeAllRemote();
    m_connected = false;
}


void
AccountManager::toggleAccountsConnected()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( m_connected )
        disconnectAll();
    else
        connectAll();
}


void
AccountManager::loadFromConfig()
{
    m_creds = new CredentialsManager( this );

    ConfigStorage* localCS = new LocalConfigStorage( this );
    m_configStorageById.insert( localCS->id(), localCS );

    QList< QObject* > configStoragePlugins = Tomahawk::Utils::PluginLoader( "configstorage" ).loadPlugins().values();
    foreach( QObject* plugin, configStoragePlugins )
    {
        ConfigStorage* cs = qobject_cast< ConfigStorage* >( plugin );
        if ( !cs )
            continue;

        m_configStorageById.insert( cs->id(), cs );
    }

    foreach ( ConfigStorage* cs, m_configStorageById )
    {
        m_configStorageLoading.insert( cs->id() );
        NewClosure( cs, SIGNAL( ready() ),
                    this, SLOT( finishLoadingFromConfig( QString ) ), cs->id() );
        cs->init();
    }
}


void
AccountManager::finishLoadingFromConfig( const QString& csid )
{
    if ( m_configStorageLoading.contains( csid ) )
        m_configStorageLoading.remove( csid );

    if ( !m_configStorageLoading.isEmpty() )
        return;

    // First we prioritize available ConfigStorages
    QList< ConfigStorage* > csByPriority;
    foreach ( ConfigStorage* cs, m_configStorageById )
    {
        int i = 0;
        for ( ; i < csByPriority.length(); ++i )
        {
            if ( csByPriority.at( i )->priority() > cs->priority() )
                break;
        }
        csByPriority.insert( i, cs );
    }

    // And we deduplicate
    for ( int i = 1; i < csByPriority.length(); ++i )
    {
        for ( int j = 0; j < i; ++j )
        {
            ConfigStorage* prioritized = csByPriority.value( j );
            ConfigStorage* trimming    = csByPriority.value( i );
            trimming->deduplicateFrom( prioritized );
        }
    }

    foreach ( const ConfigStorage* cs, m_configStorageById )
    {
        QStringList accountIds = cs->accountIds();

        qDebug() << "LOADING ALL ACCOUNTS FOR STORAGE" << cs->metaObject()->className()
                 << ":" << accountIds;

        foreach ( const QString& accountId, accountIds )
        {
            QString pluginFactory = factoryFromId( accountId );
            if ( m_accountFactories.contains( pluginFactory ) )
            {
                Account* account = loadPlugin( accountId );
                if ( account )
                    addAccount( account );
            }
        }
    }
    m_readyForSip = true;
    emit readyForSip(); //we have to yield to TomahawkApp because we don't know if Servent is ready
}


void
AccountManager::initSIP()
{
    tDebug() << Q_FUNC_INFO;
    foreach ( Account* account, accounts() )
    {
        hookupAndEnable( account, true );
    }

    m_completelyReady = true;
    emit ready();
}


Account*
AccountManager::loadPlugin( const QString& accountId )
{
    QString factoryName = factoryFromId( accountId );

    Q_ASSERT( m_accountFactories.contains( factoryName ) );
    if ( !m_accountFactories.contains( factoryName ) )
        return 0;

    Account* account = m_accountFactories[ factoryName ]->createAccount( accountId );
    if ( !account )
        return 0;

    hookupAccount( account );
    return account;
}


void
AccountManager::addAccount( Account* account )
{
    tDebug() << Q_FUNC_INFO << "adding account plugin" << account->accountId();
    m_accounts.append( account );

    if ( account->types() & Accounts::SipType )
        m_accountsByAccountType[ Accounts::SipType ].append( account );
    if ( account->types() & Accounts::InfoType )
        m_accountsByAccountType[ Accounts::InfoType ].append( account );
    if ( account->types() & Accounts::ResolverType )
        m_accountsByAccountType[ Accounts::ResolverType ].append( account );
    if ( account->types() & Accounts::StatusPushType )
        m_accountsByAccountType[ Accounts::StatusPushType ].append( account );

    emit added( account );
}


void
AccountManager::removeAccount( Account* account )
{
    account->deauthenticate();

    // emit before moving from list so accountmodel can get indexOf
    emit removed( account );

    m_accounts.removeAll( account );
    m_enabledAccounts.removeAll( account );
    m_connectedAccounts.removeAll( account );
    foreach ( AccountType type, m_accountsByAccountType.keys() )
    {
        QList< Account* > accounts = m_accountsByAccountType.value( type );
        accounts.removeAll( account );
        m_accountsByAccountType[ type ] = accounts;
    }

    ResolverAccount* raccount = qobject_cast< ResolverAccount* >( account );
    if ( raccount )
        raccount->removeBundle();

    TomahawkSettings::instance()->removeAccount( account->accountId() );

    account->removeFromConfig();
    account->deleteLater();
}


QList< Account* >
AccountManager::accountsFromFactory( AccountFactory* factory ) const
{
    QList< Account* > accts;
    foreach ( Account* acct, m_accounts )
    {
        if ( factoryForAccount( acct ) == factory )
            accts << acct;
    }
    return accts;
}


Account*
AccountManager::accountFromPath( const QString& accountPath )
{
    foreach ( AccountFactory* factory, m_factoriesForFilesytem )
    {
        if ( factory->acceptsPath( accountPath ) )
        {
            return factory->createFromPath( accountPath );
        }
    }

    Q_ASSERT_X( false, "Shouldn't have had no account factory accepting a path.. at least ResolverAccount!", "" );
    return 0;
}


void
AccountManager::registerAccountFactoryForFilesystem( AccountFactory* factory )
{
    m_factoriesForFilesytem.prepend( factory );
}


void
AccountManager::addAccountFactory( AccountFactory* factory )
{
    m_accountFactories[ factory->factoryId() ] = factory;
}


Account*
AccountManager::zeroconfAccount() const
{
    foreach ( Account* account, accounts() )
    {
        if ( account->sipPlugin( false ) && account->sipPlugin()->serviceName() == "zeroconf" )
            return account;
    }

    return 0;
}


ConfigStorage*
AccountManager::configStorageForAccount( const QString& accountId )
{
    foreach ( ConfigStorage* cs, m_configStorageById )
    {
        if ( cs->accountIds().contains( accountId ) )
            return cs;
    }
    tLog() << "Warning: defaulting to LocalConfigStorage for account" << accountId;
    return localConfigStorage();
}


ConfigStorage*
AccountManager::localConfigStorage()
{
    return m_configStorageById.value( "localconfigstorage" );
}


void
AccountManager::hookupAccount( Account* account ) const
{
    connect( account, SIGNAL( error( int, QString ) ), SLOT( onError( int, QString ) ), Qt::UniqueConnection );
    connect( account, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), SLOT( onStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), Qt::UniqueConnection );
}


void
AccountManager::hookupAndEnable( Account* account, bool startup )
{
    Q_UNUSED( startup );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    hookupAccount( account );
    if ( account->enabled() )
    {
        account->authenticate();
        m_enabledAccounts << account;
    }
}


void
AccountManager::onError( int code, const QString& msg )
{
    Account* account = qobject_cast< Account* >( sender() );
    Q_ASSERT( account );


    qWarning() << "Failed to connect to SIP:" << account->accountFriendlyName() << code << msg;

    SipStatusMessage* statusMessage;
    if ( code == Account::AuthError )
    {
        statusMessage = new SipStatusMessage( SipStatusMessage::SipLoginFailure, account->accountFriendlyName() );
        JobStatusView::instance()->model()->addJob( statusMessage );
    }
    else
    {
        statusMessage = new SipStatusMessage(SipStatusMessage::SipConnectionFailure, account->accountFriendlyName(), msg );
        JobStatusView::instance()->model()->addJob( statusMessage );
        QTimer::singleShot( 10000, account, SLOT( authenticate() ) );
    }
}


void
AccountManager::onSettingsChanged()
{
    foreach ( Account* account, m_accounts )
    {
        if ( account->types() & Accounts::SipType && account->sipPlugin( false ) )
            account->sipPlugin()->checkSettings();
    }
}


void
AccountManager::onStateChanged( Account::ConnectionState state )
{
    Account* account = qobject_cast< Account* >( sender() );
    Q_ASSERT( account );

    if ( account->connectionState() == Account::Disconnected )
    {
        m_connectedAccounts.removeAll( account );
        DisconnectReason reason = account->enabled() ? Disconnected : Disabled;
        emit disconnected( account, reason );
    }
    else if ( account->connectionState() == Account::Connected )
    {
        m_connectedAccounts << account;
        emit connected( account );
    }

    emit stateChanged( account, state );
}


}

}
