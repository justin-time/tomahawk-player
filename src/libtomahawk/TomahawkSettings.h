/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012  Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef TOMAHAWK_SETTINGS_H
#define TOMAHAWK_SETTINGS_H

#include "DllMacro.h"
#include "Typedefs.h"

#include "network/Enums.h"
#include "AtticaManager.h"

#include <QSettings>
#include <QNetworkProxy>
#include <QStringList>

#define TOMAHAWK_SETTINGS_VERSION 17

/**
 * Convenience wrapper around QSettings for tomahawk-specific config
 */
class DLLEXPORT TomahawkSettings : public QSettings
{
Q_OBJECT

public:
    static TomahawkSettings* instance();

    explicit TomahawkSettings( QObject* parent = 0 );
    virtual ~TomahawkSettings();

    void applyChanges() { emit changed(); }

    /// General settings
    QString storageCacheLocation() const;

    QStringList scannerPaths() const; /// QDesktopServices::MusicLocation in TomahawkSettingsGui
    void setScannerPaths( const QStringList& paths );
    bool hasScannerPaths() const;
    uint scannerTime() const;
    void setScannerTime( uint time );

    uint infoSystemCacheVersion() const;
    void setInfoSystemCacheVersion( uint version );
    uint genericCacheVersion() const;
    void setGenericCacheVersion( uint version );

    bool watchForChanges() const;
    void setWatchForChanges( bool watch );

    bool acceptedLegalWarning() const;
    void setAcceptedLegalWarning( bool accept );

    /// UI settings
    QByteArray mainWindowGeometry() const;
    void setMainWindowGeometry( const QByteArray& geom );

    QByteArray mainWindowState() const;
    void setMainWindowState( const QByteArray& state );

    QByteArray mainWindowSplitterState() const;
    void setMainWindowSplitterState( const QByteArray& state );

    bool verboseNotifications() const;
    void setVerboseNotifications( bool notifications );

    bool menuBarVisible() const;
    void setMenuBarVisible( bool visible );

    bool fullscreenEnabled() const;
    void setFullscreenEnabled( bool fullscreen );

    // Collection Stuff
    bool showOfflineSources() const;
    void setShowOfflineSources( bool show );

    bool enableEchonestCatalogs() const;
    void setEnableEchonestCatalogs( bool enable );

    /// Audio stuff
    unsigned int volume() const;
    void setVolume( unsigned int volume );

    /// Playlist stuff
    QByteArray playlistColumnSizes( const QString& playlistid ) const;
    void setPlaylistColumnSizes( const QString& playlistid, const QByteArray& state );

    QStringList recentlyPlayedPlaylistGuids( unsigned int amount = 0 ) const;
    void appendRecentlyPlayedPlaylist( const QString& playlistGuid, int sourceId );

    bool shuffleState( const QString& playlistid ) const;
    void setShuffleState( const QString& playlistid, bool state );
    Tomahawk::PlaylistModes::RepeatMode repeatMode( const QString& playlistid );
    void setRepeatMode( const QString& playlistid, Tomahawk::PlaylistModes::RepeatMode mode );

    // remove shuffle state and repeat state
    void removePlaylistSettings( const QString& playlistid );

    QVariant queueState() const;
    void setQueueState( const QVariant& state );

    /// SIP plugins
    // all plugins we know about. loaded, unloaded, enabled, disabled.
    void setSipPlugins( const QStringList& plugins );
    QStringList sipPlugins() const;

    // just the enabled sip plugins.
    void setEnabledSipPlugins( const QStringList& list );
    QStringList enabledSipPlugins() const;
    void enableSipPlugin( const QString& pluginId );
    void disableSipPlugin( const QString& pluginId );

    void addSipPlugin( const QString& pluginId, bool enable = true );
    void removeSipPlugin( const QString& pluginId );

    void setAccounts( const QStringList& accountIds );
    QStringList accounts() const;
    void addAccount( const QString& accountId );
    void removeAccount( const QString& accountId );

    void setBookmarkPlaylist( const QString& guid );
    QString bookmarkPlaylist() const;

    /// Network settings
    Tomahawk::Network::ExternalAddress::Mode externalAddressMode();
    void setExternalAddressMode( Tomahawk::Network::ExternalAddress::Mode externalAddressMode );

    bool httpEnabled() const; /// true by default
    void setHttpEnabled( bool enable );

    bool httpBindAll() const; /// false by default
    void setHttpBindAll( bool bindAll );

    bool crashReporterEnabled() const; /// true by default
    void setCrashReporterEnabled( bool enable );

    bool songChangeNotificationEnabled() const; /// true by default
    void setSongChangeNotificationEnabled( bool enable );

    bool autoDetectExternalIp() const;
    void setAutoDetectExternalIp( bool autoDetect );

    QString externalHostname() const;
    void setExternalHostname( const QString& externalHostname );

    int defaultPort() const;
    int externalPort() const;
    void setExternalPort( int externalPort );

    QString proxyHost() const;
    void setProxyHost( const QString& host );
    QString proxyNoProxyHosts() const;
    void setProxyNoProxyHosts( const QString& hosts );
    qulonglong proxyPort() const;
    void setProxyPort( const qulonglong port );
    QString proxyUsername() const;
    void setProxyUsername( const QString& username );
    QString proxyPassword() const;
    void setProxyPassword( const QString& password );
    QNetworkProxy::ProxyType proxyType() const;
    void setProxyType( const QNetworkProxy::ProxyType type );
    bool proxyDns() const;
    void setProxyDns( bool lookupViaProxy );

    /// ACL settings
    QVariantList aclEntries() const;
    void setAclEntries( const QVariantList& entries );

    bool isSslCertKnown( const QByteArray& sslDigest ) const;
    bool isSslCertTrusted( const QByteArray& sslDigest ) const;
    void setSslCertTrusted( const QByteArray& sslDigest, bool trusted );

    /// XMPP Component Settings
    QString xmppBotServer() const;
    void setXmppBotServer( const QString& server );
    QString xmppBotJid() const;
    void setXmppBotJid( const QString& component );
    QString xmppBotPassword() const;
    void setXmppBotPassword( const QString& password );
    int xmppBotPort() const;
    void setXmppBotPort( const int port );

    QString scriptDefaultPath() const;
    void setScriptDefaultPath( const QString& path );
    QString playlistDefaultPath() const;
    void setPlaylistDefaultPath( const QString& path );

    // Now-Playing Settings
    // For now, just Adium. Soon, the world!
    bool nowPlayingEnabled() const; // false by default
    void setNowPlayingEnabled( bool enable );

    enum PrivateListeningMode
    {
        PublicListening,
        NoLogPlayback,
        FullyPrivate
    };
    PrivateListeningMode privateListeningMode() const;
    void setPrivateListeningMode( PrivateListeningMode mode );

    void setImportPlaylistPath( const QString& path );
    QString importPlaylistPath() const;

    Tomahawk::SerializedUpdaters playlistUpdaters() const;
    void setPlaylistUpdaters( const Tomahawk::SerializedUpdaters& updaters );

    static void registerCustomSettingsHandlers();

    // Charts
    void setLastChartIds( const QMap<QString, QVariant>& ids );
    QMap<QString, QVariant> lastChartIds();

    AtticaManager::StateHash atticaResolverStates() const;
    void setAtticaResolverStates( const AtticaManager::StateHash states );

    void setAtticaResolverState( const QString& resolver, AtticaManager::ResolverState state );
    void removeAtticaResolverState( const QString& resolver );

    // Playdar TLS Certificate and Key.
    // TODO: Store in Keychain
    QByteArray playdarCertificate() const;
    void setPlaydarCertificate( const QByteArray& cert );
    QByteArray playdarKey() const;
    void setPlaydarKey( const QByteArray& key );

signals:
    void changed();
    void recentlyPlayedPlaylistAdded( const QString& playlistId, int sourceId );

private slots:
    void updateIndex();

private:
    void doInitialSetup();
    void createLastFmAccount();
    void createSpotifyAccount();
    void doUpgrade( int oldVersion, int newVersion );

    static TomahawkSettings* s_instance;
};

Q_DECLARE_METATYPE( TomahawkSettings::PrivateListeningMode )
Q_DECLARE_METATYPE( AtticaManager::StateHash )

#endif
