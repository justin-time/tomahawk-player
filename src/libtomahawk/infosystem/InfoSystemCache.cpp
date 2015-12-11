/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include <QtDebug>
#include <QDesktopServices>

#include "InfoSystemCache.h"
#include "TomahawkSettings.h"
#include "utils/Logger.h"
#include "Source.h"

#include <QDir>
#include <QSettings>
#include <QCryptographicHash>

namespace Tomahawk
{

namespace InfoSystem
{

const int InfoSystemCache::s_infosystemCacheVersion = 4;

InfoSystemCache::InfoSystemCache( QObject* parent )
    : QObject( parent )
    , m_cacheBaseDir( TomahawkSettings::instance()->storageCacheLocation() + "/InfoSystemCache/" )
{
    tDebug() << Q_FUNC_INFO;

    if ( TomahawkSettings::instance()->infoSystemCacheVersion() < s_infosystemCacheVersion )
    {
        TomahawkUtils::removeDirectory( m_cacheBaseDir );
        TomahawkSettings::instance()->setInfoSystemCacheVersion( s_infosystemCacheVersion );
    }

    m_pruneTimer.setInterval( 300000 );
    m_pruneTimer.setSingleShot( false );
    connect( &m_pruneTimer, SIGNAL( timeout() ), SLOT( pruneTimerFired() ) );
    m_pruneTimer.start();
}


InfoSystemCache::~InfoSystemCache()
{
    tDebug() << Q_FUNC_INFO;
}


void
InfoSystemCache::pruneTimerFired()
{
    qDebug() << Q_FUNC_INFO << "Pruning infosystemcache";
    qlonglong currentMSecsSinceEpoch = QDateTime::currentMSecsSinceEpoch();

    for ( int i = InfoNoInfo; i <= InfoLastInfo; i++ )
    {
        InfoType type = (InfoType)(i);
        QHash< QString, QString > fileLocationHash = m_fileLocationCache[type];
        const QString cacheDirName = m_cacheBaseDir + QString::number( (int)type );
        QFileInfoList fileList = QDir( cacheDirName ).entryInfoList( QDir::Files | QDir::NoDotAndDotDot );
        foreach ( QFileInfo file, fileList )
        {
            QString baseName = file.baseName();
            if ( file.suffix().toLongLong() < currentMSecsSinceEpoch )
            {
                if ( !QFile::remove( file.canonicalFilePath() ) )
                    tLog() << "Failed to remove stale cache file" << file.canonicalFilePath();
                else
                    qDebug() << "Removed stale cache file" << file.canonicalFilePath();
            }
            if ( fileLocationHash.contains( baseName ) )
                fileLocationHash.remove( baseName );
        }
        m_fileLocationCache[type] = fileLocationHash;
    }
}


void
InfoSystemCache::getCachedInfoSlot( Tomahawk::InfoSystem::InfoStringHash criteria, qint64 newMaxAge, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    QObject* sendingObj = sender();
    const QString criteriaHashVal = criteriaMd5( criteria );
    const QString criteriaHashValWithType = criteriaMd5( criteria, requestData.type );
    QHash< QString, QString > fileLocationHash = m_fileLocationCache[ requestData.type ];
    if ( !fileLocationHash.contains( criteriaHashVal ) )
    {
        if ( !fileLocationHash.isEmpty() )
        {
            //We already know of some values, so no need to re-read the directory again as it's already happened
            //qDebug() << Q_FUNC_INFO << "notInCache -- filelocationhash empty";
            notInCache( sendingObj, criteria, requestData );
            return;
        }

        const QString cacheDir = m_cacheBaseDir + QString::number( (int)requestData.type );
        QDir dir( cacheDir );
        if ( !dir.exists() )
        {
            //Dir doesn't exist so clearly not in cache
            //qDebug() << Q_FUNC_INFO << "notInCache -- dir doesn't exist";
            notInCache( sendingObj, criteria, requestData );
            return;
        }

        QFileInfoList fileList = dir.entryInfoList( QDir::Files | QDir::NoDotAndDotDot );
        foreach ( QFileInfo file, fileList )
        {
            QString baseName = file.baseName();
            fileLocationHash[ baseName ] = file.canonicalFilePath();
        }

        //Store what we've loaded up
        m_fileLocationCache[ requestData.type ] = fileLocationHash;
        if ( !fileLocationHash.contains( criteriaHashVal ) )
        {
            //Still didn't find it? It's really not in the cache then
            //qDebug() << Q_FUNC_INFO << "notInCache -- filelocationhash doesn't contain criteria val";
            notInCache( sendingObj, criteria, requestData );
            return;
        }
    }

    QFileInfo file( fileLocationHash[ criteriaHashVal ] );
    qlonglong currMaxAge = file.suffix().toLongLong();

    if ( currMaxAge < QDateTime::currentMSecsSinceEpoch() )
    {
        if ( !QFile::remove( file.canonicalFilePath() ) )
            tLog() << "Failed to remove stale cache file" << file.canonicalFilePath();
        else
            qDebug() << "Removed stale cache file" << file.canonicalFilePath();

        fileLocationHash.remove( criteriaHashVal );
        m_fileLocationCache[ requestData.type ] = fileLocationHash;
        m_dataCache.remove( criteriaHashValWithType );

        qDebug() << Q_FUNC_INFO << "notInCache -- file was stale";
        notInCache( sendingObj, criteria, requestData );
        return;
    }
    else if ( newMaxAge > 0 )
    {
        const QString newFilePath = QString( file.dir().canonicalPath() + '/' + criteriaHashVal + '.' + QString::number( QDateTime::currentMSecsSinceEpoch() + newMaxAge ) );

        if ( !QFile::rename( file.canonicalFilePath(), newFilePath ) )
        {
            qDebug() << Q_FUNC_INFO << "notInCache -- failed to move old cache file to new location";
            notInCache( sendingObj, criteria, requestData );
            return;
        }

        fileLocationHash[ criteriaHashVal ] = newFilePath;
        m_fileLocationCache[ requestData.type ] = fileLocationHash;
    }

    if ( !m_dataCache.contains( criteriaHashValWithType ) )
    {
        QSettings cachedSettings( fileLocationHash[ criteriaHashVal ], QSettings::IniFormat );
        QVariant output = cachedSettings.value( "data" );
        m_dataCache.insert( criteriaHashValWithType, new QVariant( output ) );

        emit info( requestData, output );
    }
    else
    {
        emit info( requestData, QVariant( *( m_dataCache[ criteriaHashValWithType ] ) ) );
    }
}


void
InfoSystemCache::notInCache( QObject *receiver, Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    QMetaObject::invokeMethod( receiver, "notInCacheSlot", Q_ARG( Tomahawk::InfoSystem::InfoStringHash, criteria ), Q_ARG( Tomahawk::InfoSystem::InfoRequestData, requestData ) );
}


void
InfoSystemCache::updateCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, qint64 maxAge, Tomahawk::InfoSystem::InfoType type, QVariant output )
{
    const QString criteriaHashVal = criteriaMd5( criteria );
    const QString criteriaHashValWithType = criteriaMd5( criteria, type );
    const QString cacheDir = m_cacheBaseDir + QString::number( (int)type );
    const QString settingsFilePath( cacheDir + '/' + criteriaHashVal + '.' + QString::number( QDateTime::currentMSecsSinceEpoch() + maxAge ) );

    QHash< QString, QString > fileLocationHash = m_fileLocationCache[ type ];
    if ( fileLocationHash.contains( criteriaHashVal ) )
    {
        if ( !QFile::rename( fileLocationHash[ criteriaHashVal ], settingsFilePath ) )
        {
            tLog() << "Failed to move old cache file to new location!";
            return;
        }
        fileLocationHash[ criteriaHashVal ] = settingsFilePath;
        m_fileLocationCache[ type ] = fileLocationHash;

        QSettings cachedSettings( fileLocationHash[ criteriaHashVal ], QSettings::IniFormat );
        cachedSettings.setValue( "data", output );

        m_dataCache.insert( criteriaHashValWithType, new QVariant( output ) );

        return;
    }

    QDir dir( cacheDir );
    if( !dir.exists( cacheDir ) )
    {
        qDebug() << "Creating cache directory" << cacheDir;
        if( !dir.mkpath( cacheDir ) )
        {
            tLog() << "Failed to create cache dir! Bailing...";
            return;
        }
    }

    QSettings cachedSettings( settingsFilePath, QSettings::IniFormat );
    QStringList keys = criteria.keys();
    cachedSettings.beginGroup( "criteria" );
    for( int i = 0; i < criteria.size(); i++ )
        cachedSettings.setValue( keys.at( i ), criteria[keys.at( i )] );
    cachedSettings.endGroup();
    cachedSettings.setValue( "data", output );

    fileLocationHash[criteriaHashVal] = settingsFilePath;
    m_fileLocationCache[type] = fileLocationHash;
    m_dataCache.insert( criteriaHashValWithType, new QVariant( output ) );
}


const QString
InfoSystemCache::criteriaMd5( const Tomahawk::InfoSystem::InfoStringHash &criteria, Tomahawk::InfoSystem::InfoType type ) const
{
    QCryptographicHash md5( QCryptographicHash::Md5 );
    QStringList keys = criteria.keys();
    keys.sort();
    foreach( QString key, keys )
    {
        md5.addData( key.toUtf8() );
        md5.addData( criteria[key].toUtf8() );
    }
    if ( type != Tomahawk::InfoSystem::InfoNoInfo && type != Tomahawk::InfoSystem::InfoLastInfo )
        md5.addData( QString::number( (int)type ).toUtf8() );
    return md5.result().toHex();
}


} //namespace InfoSystem

} //namespace Tomahawk
