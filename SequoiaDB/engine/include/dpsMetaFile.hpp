/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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

#pragma pack(4)

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

   #define DPS_INVALID_FILE_SN               ( (UINT32)~0 )

   /*
      _dpsMetaFileContent define
   */
   struct _dpsMetaFileContent
   {
      DPS_LSN_OFFSET _oldestLSNOffset ;
      UINT32         _beginFile ;
      UINT32         _workFile ;
      DPS_LSN_VER    _curLsnVersion ;
      UINT32         _curLsnLength ;
      DPS_LSN_OFFSET _curLsnOffset ;
      DPS_LSN_VER    _memBeginLsnVer ;
      UINT32         _reserved ;
      DPS_LSN_OFFSET _memBeginLsnOffset ;
      // 48 == sizeof(_dpsMetaFileContent)
      CHAR           _padding [ DPS_METAFILE_CONTENT_LEN - 48 ] ;

      _dpsMetaFileContent ( DPS_LSN_OFFSET offset = DPS_INVALID_LSN_OFFSET )
      {
         resetStatus() ;

         _oldestLSNOffset = offset ;
         _reserved = 0 ;
         ossMemset( _padding, 0, sizeof(_padding) ) ;

         SDB_ASSERT( sizeof(_dpsMetaFileContent) == DPS_METAFILE_CONTENT_LEN,
                     "Dps meta file content size must be 4K" ) ;
      }

      void  resetStatus()
      {
         _beginFile        = DPS_INVALID_FILE_SN ;
         _workFile         = DPS_INVALID_FILE_SN ;
         _curLsnVersion    = DPS_INVALID_LSN_VERSION ;
         _curLsnLength     = 0 ;
         _curLsnOffset     = DPS_INVALID_LSN_OFFSET ;
         _memBeginLsnVer   = DPS_INVALID_LSN_VERSION ;
         _memBeginLsnOffset= DPS_INVALID_LSN_OFFSET ;
      }

      void  reset()
      {
         _oldestLSNOffset  = DPS_INVALID_LSN_OFFSET ;
         resetStatus() ;
      }

      DPS_LSN_OFFSET getOldestLSNOffset() const
      {
         return _oldestLSNOffset ;
      }

      BOOLEAN isStatusValid() const
      {
         if ( DPS_INVALID_FILE_SN == _beginFile ||
              DPS_INVALID_FILE_SN == _workFile ||
              DPS_INVALID_LSN_VERSION == _curLsnVersion ||
              0 == _curLsnLength ||
              DPS_INVALID_LSN_OFFSET == _curLsnOffset )
         {
            return FALSE ;
         }
         return TRUE ;
      }

   } ;

   typedef _dpsMetaFileContent dpsMetaFileContent ;

#pragma pack()

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
      INT32 stop( DPS_LSN_OFFSET oldestTransLSN,
                  UINT32 beginFile,
                  UINT32 workFile,
                  const DPS_LSN &curLSN,
                  UINT32 curLsnLength,
                  const DPS_LSN &memBeginLSN ) ;

      INT32 invalidateStatus() ;
      INT32 writeOldestLSNOffset( DPS_LSN_OFFSET offset,
                                  BOOLEAN needSync = TRUE ) ;

      DPS_LSN_OFFSET getCacheLSN() const { return _content._oldestLSNOffset ; }
      BOOLEAN        isCacheLSNValid() const ;
      BOOLEAN        hasInvalidateStatus() const { return _invalidateStatus ; }

      dpsMetaFileContent  getContent() const { return _content ; }

   private:
      INT32 _initNewFile() ;
      INT32 _restore() ;
      INT32 writeContent( BOOLEAN needSync = TRUE ) ;
      INT32 readContent() ;

   private:
      _OSS_FILE          _file ;
      CHAR               _path[ OSS_MAX_PATHSIZE + 1 ] ;
      _dpsMetaFileHeader _header ;
      dpsMetaFileContent _content ;
      BOOLEAN            _invalidateStatus ;
   } ;
}

#endif

