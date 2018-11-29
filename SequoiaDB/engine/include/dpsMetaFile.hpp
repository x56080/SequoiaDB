/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = dpsMetaFile.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2018  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSMETAFILE_H_
#define DPSMETAFILE_H_

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"
#include "dpsLogDef.hpp"

using namespace std ;

namespace engine
{
   #define DPS_METAFILE_NAME                   "sequoiadbLog.meta"

   #define DPS_METAFILE_HEADER_EYECATCHER      "DPSMETAH"
   #define DPS_METAFILE_HEADER_EYECATCHER_LEN  8
   #define DPS_METAFILE_HEADER_LEN             (64 * 1024)

   #define DPS_METAFILE_CONTENT_LEN            (4 * 1024)

   #define DPS_METAFILE_VERSION1       (1)

   /*
      _dpsMetaFileHeader define
   */
   struct _dpsMetaFileHeader
   {
      CHAR     _eyeCatcher[ DPS_METAFILE_HEADER_EYECATCHER_LEN ] ;
      UINT32   _version ;
      // 12 == (sizeof(_version) + sizeof(_eyeCatcher))
      CHAR     _padding [ DPS_METAFILE_HEADER_LEN - 12 ] ;

      _dpsMetaFileHeader ()
      {
         ossMemcpy( _eyeCatcher, DPS_METAFILE_HEADER_EYECATCHER,
                    DPS_METAFILE_HEADER_EYECATCHER_LEN) ;
         _version = DPS_METAFILE_VERSION1 ;
         ossMemset( _padding, 0, sizeof(_padding) ) ;

         SDB_ASSERT( sizeof(_dpsMetaFileHeader) == DPS_METAFILE_HEADER_LEN,
                     "Dps meta file header size must be 64K" ) ;
      }
   } ;

   struct _dpsMetaFileContent
   {
      DPS_LSN_OFFSET _oldestLSNOffset ;
      // 8 == sizeof(_oldestLSNOffset)
      CHAR           _padding [ DPS_METAFILE_CONTENT_LEN - 8 ] ;

      _dpsMetaFileContent ( DPS_LSN_OFFSET offset )
      {
         _oldestLSNOffset = offset ;
         ossMemset( _padding, 0, sizeof(_padding) ) ;

         SDB_ASSERT( sizeof(_dpsMetaFileContent) == DPS_METAFILE_CONTENT_LEN,
                     "Dps meta file content size must be 4K" ) ;
      }

      DPS_LSN_OFFSET getOldestLSNOffset()
      {
         return _oldestLSNOffset ;
      }
   } ;

   /*
      _dpsMetaFile define
   */
   class _dpsMetaFile : public SDBObject
   {
   public:
      _dpsMetaFile() ;
      ~_dpsMetaFile() ;

   public:
      INT32 init( const CHAR *parentDir ) ;

      INT32 writeOldestLSNOffset( DPS_LSN_OFFSET offset ) ;

      INT32 readOldestLSNOffset( DPS_LSN_OFFSET &offset ) ;

      INT32 sync() ;

   private:
      INT32 _initNewFile() ;
      INT32 _restore() ;

   private:
      _OSS_FILE          _file ;
      CHAR               _path[ OSS_MAX_PATHSIZE + 1 ] ;
      _dpsMetaFileHeader _header ;
   } ;
}

#endif

