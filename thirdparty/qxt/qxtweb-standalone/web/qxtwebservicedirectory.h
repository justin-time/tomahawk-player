
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

#ifndef QXTWEBSERVICEDIRECTORY_H
#define QXTWEBSERVICEDIRECTORY_H

#include <qxtabstractwebservice.h>
#include <QString>
class QxtAbstractWebSessionManager;
class QxtWebEvent;

class QxtWebServiceDirectoryPrivate;
class QXT_WEB_EXPORT QxtWebServiceDirectory : public QxtAbstractWebService
{
    Q_OBJECT
public:
    explicit QxtWebServiceDirectory(QxtAbstractWebSessionManager* sm, QObject* parent = 0);

    void addService(const QString& path, QxtAbstractWebService* service);
    void removeService(const QString& path);
    QxtAbstractWebService* service(const QString& path) const;

    virtual void pageRequestedEvent(QxtWebRequestEvent* event);
//     virtual void functionInvokedEvent(QxtWebRequestEvent* event);

    QString defaultRedirect() const;
    void setDefaultRedirect(const QString& path);

protected:
    virtual void unknownServiceRequested(QxtWebRequestEvent* event, const QString& name);
    virtual void indexRequested(QxtWebRequestEvent* event);

private:
    QXT_DECLARE_PRIVATE(QxtWebServiceDirectory)
};

#endif // QXTWEBSERVICEDIRECTORY_H
