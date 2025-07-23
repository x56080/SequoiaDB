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

   Source File Name = utilRenameLogger.hpp

   Descriptive Name = util rename cs cl

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== ======== ==============================================
          12/17/2018  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_RENAMELOGGER_HPP_
#define UTIL_RENAMELOGGER_HPP_

#include <vector>
#include "ossFile.hpp"
#include "ossUtil.hpp"
#include "dms.hpp"

namespace engine
{
   #define UTIL_RENAME_LOG_FILENAME     ".SEQUOIADB_RENAME_INFO"
   #define UTIL_RENAME_LOG_FILESIZE_MAX 512
   #define UTIL_RENAME_LOG_SEP          '='
   #define UTIL_RENAME_LOG_OLDNAME      "oldname"
   #define UTIL_RENAME_LOG_NEWNAME      "newname"
   #define UTIL_RENAME_LOG_FIELD_NUM    2

   struct _utilRenameLog
   {
      CHAR oldName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      CHAR newName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;

      _utilRenameLog()
      {
         ossMemset( oldName, 0, sizeof( oldName ) ) ;
         ossMemset( newName, 0, sizeof( newName ) ) ;
      }

      _utilRenameLog( const CHAR* oldname, const CHAR* newname )
      {
         ossMemset( oldName, 0, sizeof( oldName ) ) ;
         ossStrncpy( oldName, oldname, DMS_COLLECTION_SPACE_NAME_SZ ) ;
         ossMemset( newName, 0, sizeof( newName ) ) ;
         ossStrncpy( newName, newname, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      }

      void toString( CHAR* strLog, UINT32 size ) const
      {
         ossSnprintf( strLog, size - 1,
                      "%s%c%s"OSS_NEWLINE
                      "%s%c%s"OSS_NEWLINE,
                      UTIL_RENAME_LOG_OLDNAME, UTIL_RENAME_LOG_SEP, oldName,
                      UTIL_RENAME_LOG_NEWNAME, UTIL_RENAME_LOG_SEP, newName ) ;
      }

      BOOLEAN isValid() const
      {
         return 0 != oldName[ 0 ] && 0 != newName[ 0 ] ;
      }

      void clear()
      {
         oldName[ 0 ] = 0 ;
         newName[ 0 ] = 0 ;
      }
   } ;
   typedef _utilRenameLog utilRenameLog ;

   BOOLEAN utilStr2RenameLog( const string& str, utilRenameLog& log ) ;

   enum UTIL_RENAME_LOGGER_MODE
   {
      UTIL_RENAME_LOGGER_WRITE = 0,
      UTIL_RENAME_LOGGER_READ
   } ;

   class _utilRenameLogger : public SDBObject
   {
      public:
         _utilRenameLogger () ;
         ~_utilRenameLogger () ;

         INT32 init( UTIL_RENAME_LOGGER_MODE mode = UTIL_RENAME_LOGGER_WRITE ) ;
         INT32 init( const CHAR *fileName,
                     UTIL_RENAME_LOGGER_MODE mode ) ;
         INT32 log( const utilRenameLog& log ) ;
         INT32 load( utilRenameLog& log );
         INT32 clear() ;
         const CHAR* fileName() ;
         BOOLEAN isOpened() const ;

      private:
         BOOLEAN _isOpened ;
         OSSFILE _file ;
         CHAR _fileName[ OSS_MAX_PATHSIZE + 1 ] ;
         BOOLEAN _fileExist ;
   } ;
   typedef _utilRenameLogger utilRenameLogger ;

   /*
      _utilRenameLogManager define
    */
   class _utilRenameLogManager : public SDBObject
   {
   public:
      _utilRenameLogManager() {}
      ~_utilRenameLogManager() {}

      INT32 load() ;
      INT32 clearAll() ;
      INT32 clear( const utilRenameLog &log ) ;

      BOOLEAN hasRenamed() ;
      INT32 getRenameLog( const CHAR *csName, utilRenameLog &log ) ;

   protected:
      typedef ossPoolMap< ossPoolString, ossPoolString > _UTIL_STRING_MAP ;
      typedef _UTIL_STRING_MAP::iterator _UTIL_STRING_MAP_IT ;

      INT32 _clear( const CHAR *fileName ) ;
      static _UTIL_STRING_MAP_IT _find( const CHAR *name,
                                        _UTIL_STRING_MAP &targetMap ) ;

   protected:
      // old name -> file name mapping
      _UTIL_STRING_MAP  _oldToFileMap ;
      // old name -> new name mapping
      _UTIL_STRING_MAP  _oldToNewMap ;
      // new name -> old name mapping
      _UTIL_STRING_MAP  _newToOldMap ;
   } ;

   typedef class _utilRenameLogManager utilRenameLogManager ;

}

#endif //UTIL_RENAMELOGGER_HPP_

