/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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

#ifndef TOMAHAWKWINDOW_H
#define TOMAHAWKWINDOW_H

#include "Result.h"
#include "audio/AudioEngine.h"
#include "utils/XspfLoader.h"
#include "utils/DpiScaler.h"

#include "config.h"

#include <QMainWindow>
#include <QVariantMap>
#include <QPushButton>
#include <QString>
#include <QStackedWidget>
#include <QToolButton>
#ifdef Q_OS_WIN
    #include <shobjidl.h>
#if QT_VERSION >= QT_VERSION_CHECK( 5, 2, 0 )
    #include <QWinThumbnailToolBar>
    #include <QWinThumbnailToolButton>
#endif
#endif

namespace Tomahawk
{
    namespace Accounts
    {
        class Account;
    }
}

class JobStatusSortModel;
class QSearchField;
class SourceTreeView;
class QAction;

class SettingsDialog;
class MusicScanner;
class AudioControls;
class TomahawkTrayIcon;
class PlaylistModel;
class AnimatedSplitter;
class AccountsToolButton;

namespace Ui
{
    class TomahawkWindow;
    class GlobalSearchWidget;
}

class TomahawkWindow : public QMainWindow, private TomahawkUtils::DpiScaler
{
Q_OBJECT

public:
    TomahawkWindow( QWidget* parent = 0 );
    ~TomahawkWindow();

    AudioControls* audioControls();
    SourceTreeView* sourceTreeView() const;

    void setWindowTitle( const QString& title );

protected:
    void changeEvent( QEvent* e );
    void closeEvent( QCloseEvent* e );
    void showEvent( QShowEvent* e );
    void hideEvent( QHideEvent* e );
    void keyPressEvent( QKeyEvent* e );

    bool eventFilter( QObject* obj, QEvent* event );

#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK( 5, 2, 0 )
    bool winEvent( MSG* message, long* result );
#endif

public slots:
    void createStation();
    void createPlaylist();
    void loadPlaylist();
    void showSettingsDialog();
    void showDiagnosticsDialog();
    void legalInfo();
    void getSupport();
    void helpTranslate();
    void reportBug();
    void openLogfile();
    void updateCollectionManually();
    void rescanCollectionManually();
    void showOfflineSources();

    void fullScreenEntered();
    void fullScreenExited();

private slots:
    void onHistoryBackAvailable( bool avail );
    void onHistoryForwardAvailable( bool avail );

    void onAudioEngineError( AudioEngine::AudioErrorCode error );

    void onXSPFError( XSPFLoader::XSPFErrorCode error );
    void onJSPFError();
    void onNewPlaylistOk( const Tomahawk::playlist_ptr& );

    void onPlaybackLoading( const Tomahawk::result_ptr result );

    void audioStarted();
    void audioFinished();
    void audioPaused();
    void audioStopped();

    void showAboutTomahawk();
    void showWhatsNew_0_8();
    void checkForUpdates();

    void onSearch( const QString& search );
    void onFilterEdited();

    void loadPlaylistFinished( int );

    void minimize();
    void maximize();
    void toggleFullscreen();

    void crashNow();

    void toggleMenuBar();
    void balanceToolbar();

    void toggleLoved();

    void audioStateChanged( AudioState newState, AudioState oldState );
    void updateWindowsLoveButton();

private:
    void loadSettings();
    void saveSettings();

    void applyPlatformTweaks();
    void setupSignals();
    void setupMenuBar();
    void setupToolBar();
    void setupSideBar();
    void setupStatusBar();
    void setupShortcuts();
    void setupUpdateCheck();

    void importPlaylist( const QString& url, bool autoUpdate );

#ifdef Q_OS_WIN
    bool setupWindowsButtons();
#if QT_VERSION < QT_VERSION_CHECK( 5, 2, 0 )
    const unsigned int m_buttonCreatedID;
    HICON thumbIcon(TomahawkUtils::ImageType type);
    ITaskbarList3* m_taskbarList;
    THUMBBUTTON m_thumbButtons[5];
#else
    QIcon thumbIcon(TomahawkUtils::ImageType type);
    QWinThumbnailToolBar *m_taskbarList;
#endif
    enum TB_STATES{ TP_PREVIOUS = 0,TP_PLAY_PAUSE = 1,TP_NEXT = 2, TP_SPACE = 3, TP_LOVE = 4 };
#endif

    Ui::TomahawkWindow* ui;
    QPointer<SettingsDialog> m_settingsDialog;
    QSearchField* m_searchWidget;
    AudioControls* m_audioControls;
    TomahawkTrayIcon* m_trayIcon;
    SourceTreeView* m_sourcetree;
    QPushButton* m_statusButton;
    AnimatedSplitter* m_sidebar;
    JobStatusSortModel* m_jobsModel;

    // Menus and menu actions: Accounts menu
    QMenuBar    *m_menuBar;
#ifndef Q_OS_MAC
    QAction     *m_compactMenuAction;
    QMenu       *m_compactMainMenu;
#endif
    AccountsToolButton *m_accountsButton;
    QToolBar *m_toolbar;
    QWidget *m_toolbarLeftBalancer;
    QWidget *m_toolbarRightBalancer;

    QAction* m_backAction;
    QAction* m_forwardAction;

    Tomahawk::result_ptr m_currentTrack;
    QString m_windowTitle;
    int m_audioRetryCounter;
};

#endif // TOMAHAWKWINDOW_H
