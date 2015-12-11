/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Teo Mrnjavac <teo@kde.org>
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

#include "tag.h"

#include "apetag.h"
#include "asftag.h"
#include "id3v1tag.h"
#include "id3v2tag.h"
#include "mp4tag.h"
#include "oggtag.h"

#include <taglib/xiphcomment.h>
#include <taglib/vorbisfile.h>
#include <taglib/oggflacfile.h>
#if defined(TAGLIB_MAJOR_VERSION) && defined(TAGLIB_MINOR_VERSION)
    #if TAGLIB_MAJOR_VERSION >= 1 && TAGLIB_MINOR_VERSION >= 9
        #include <taglib/opusfile.h>
    #endif
#endif
#include <taglib/flacfile.h>
#include <taglib/speexfile.h>
#include <taglib/mpegfile.h>
#include <taglib/mp4file.h>
#include <taglib/mpcfile.h>
#include <taglib/asffile.h>
#include <taglib/aifffile.h>
#include <taglib/wavpackfile.h>

#include <QStringList>

namespace Tomahawk
{


QSharedPointer<Tag>
Tag::fromFile( const TagLib::FileRef &f )
{
    if( TagLib::Ogg::Vorbis::File *file =
            dynamic_cast< TagLib::Ogg::Vorbis::File * >( f.file() ) )
    {
        if( file->tag() )
            return QSharedPointer<Tag>( new OggTag( f.tag(), file->tag() ) );
    }
    else if( TagLib::Ogg::FLAC::File *file =
             dynamic_cast< TagLib::Ogg::FLAC::File * >( f.file() ) )
    {
        if( file->tag() )
            return QSharedPointer<Tag>( new OggTag( f.tag(), file->tag() ) );
    }
#if TAGLIB_MAJOR_VERSION >= 1 && TAGLIB_MINOR_VERSION >= 9
    else if ( TagLib::Ogg::Opus::File *file =
              dynamic_cast< TagLib::Ogg::Opus::File * >( f.file() ) )
    {
        if ( file->tag() )
        {
            return QSharedPointer<Tag>( new OggTag( f.tag(), file->tag() ) );
        }
    }
#endif
    else if( TagLib::RIFF::AIFF::File *file =
             dynamic_cast< TagLib::RIFF::AIFF::File * >( f.file() ) )
    {
        if( file->tag() )
            return QSharedPointer<Tag>( new ID3v2Tag( f.tag(), file->tag() ) );
    }
    else if( TagLib::Ogg::Speex::File *file =
             dynamic_cast< TagLib::Ogg::Speex::File * >( f.file() ) )
    {
        if( file->tag() )
            return QSharedPointer<Tag>( new OggTag( f.tag(), file->tag() ) );
    }
    else if( TagLib::FLAC::File *file =
             dynamic_cast< TagLib::FLAC::File * >( f.file() ) )
    {
        if( file->xiphComment() )
            return QSharedPointer<Tag>( new OggTag( f.tag(), file->xiphComment() ) );
        else if( file->ID3v2Tag() )
            return QSharedPointer<Tag>( new ID3v2Tag( f.tag(), file->ID3v2Tag() ) );
        else if( file->ID3v1Tag() )
            return QSharedPointer<Tag>( new ID3v1Tag( f.tag() ) );
    }
    else if( TagLib::MPEG::File *file =
             dynamic_cast< TagLib::MPEG::File * >( f.file() ) )
    {
        if( file->ID3v2Tag() )
            return QSharedPointer<Tag>( new ID3v2Tag( f.tag(), file->ID3v2Tag() ) );
        else if( file->APETag() )
            return QSharedPointer<Tag>( new APETag( f.tag(), file->APETag() ) );
        else if( file->ID3v1Tag() )
            return QSharedPointer<Tag>( new ID3v1Tag( f.tag() ) );
    }
    else if( TagLib::MP4::File *file =
             dynamic_cast< TagLib::MP4::File * >( f.file() ) )
    {
        if( file->tag() )
            return QSharedPointer<Tag>( new MP4Tag( f.tag(), file->tag() ) );
    }
    else if( TagLib::MPC::File *file =
             dynamic_cast< TagLib::MPC::File * >( f.file() ) )
    {
        if( file->APETag() )
            return QSharedPointer<Tag>( new APETag( f.tag(), file->APETag() ) );
        else if( file->ID3v1Tag() )
            return QSharedPointer<Tag>( new ID3v1Tag( f.tag() ) );
    }
    else if( TagLib::ASF::File *file =
             dynamic_cast< TagLib::ASF::File * >( f.file() ) )
    {
        if( file->tag() )
            return QSharedPointer<Tag>( new ASFTag( f.tag(), file->tag() ) );
    }
    else if( TagLib::WavPack::File *file =
        dynamic_cast< TagLib::WavPack::File * >( f.file() ) )
    {
        if( file->APETag() )
            return QSharedPointer<Tag>( new APETag( f.tag(), file->APETag() ) );
        else if( file->ID3v1Tag() )
            return QSharedPointer<Tag>( new ID3v1Tag( f.tag() ) );
    }

    return QSharedPointer<Tag>();
}

unsigned int Tag::processDiscNumber( const QString &s ) const
{
    int disc;
    if( s.indexOf( '/' ) != -1 )
        disc = s.split( '/', QString::SkipEmptyParts ).value( 0 ).toInt();
    else if( s.indexOf( ':' ) != -1 )
        disc = s.split( '/', QString::SkipEmptyParts ).value( 0 ).toInt();
    else
        disc = s.toInt();

    return disc;
}

}
