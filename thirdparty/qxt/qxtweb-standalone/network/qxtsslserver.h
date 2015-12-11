
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

#ifndef QXTSSLSERVER_H
#define QXTSSLSERVER_H

#include "qxtglobal.h"
#include <QTcpServer>

#ifndef QT_NO_OPENSSL
#include <QSslSocket>

class QxtSslServerPrivate;
class QXT_NETWORK_EXPORT QxtSslServer : public QTcpServer
{
    Q_OBJECT
public:
    QxtSslServer(QObject* parent = 0);

    virtual bool hasPendingConnections() const;
    virtual QTcpSocket* nextPendingConnection();

    void setLocalCertificate(const QSslCertificate& cert);
    void setLocalCertificate(const QString& path, QSsl::EncodingFormat format = QSsl::Pem);
    QSslCertificate localCertificate() const;

    void setPrivateKey(const QSslKey& key);
    void setPrivateKey(const QString& path, QSsl::KeyAlgorithm algo = QSsl::Rsa,
            QSsl::EncodingFormat format = QSsl::Pem, const QByteArray& passPhrase = QByteArray());
    QSslKey privateKey() const;

    void setAutoEncrypt(bool on);
    bool autoEncrypt() const;

    void setProtocol(QSsl::SslProtocol proto);
    QSsl::SslProtocol protocol() const;

protected:
#if QT_VERSION >= 0x050000
	virtual void incomingConnection(qintptr socketDescriptor);
#else
    virtual void incomingConnection(int socketDescriptor);
#endif

private:
    QXT_DECLARE_PRIVATE(QxtSslServer)
};

#endif /* QT_NO_OPENSSL */
#endif
