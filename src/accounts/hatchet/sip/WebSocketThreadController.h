/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012,      Leo Franchi <lfranchi@kde.org>
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
#ifndef WEBSOCKET_THREAD_CONTROLLER_H
#define WEBSOCKET_THREAD_CONTROLLER_H

#include <QPointer>
#include <QThread>

class WebSocket;

class WebSocketThreadController : public QThread
{
    Q_OBJECT

public:
    explicit WebSocketThreadController( QObject* sip );
    virtual ~WebSocketThreadController();

    void setUrl( const QString &url );
    void setAuthorizationHeader( const QString &authorizationHeader );

protected:
    void run();

private:
    Q_DISABLE_COPY( WebSocketThreadController )

    QPointer< WebSocket > m_webSocket;
    QPointer< QObject > m_sip;
    QString m_url;
    QString m_authorizationHeader;
};

#endif
