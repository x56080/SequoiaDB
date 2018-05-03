/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dpsLogFileMgr.hpp

   Descriptive Name = Data Protection Services Log File Manager Header

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains declare for log file manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/27/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSLOGFILEMGR_HPP__
#define DPSLOGFILEMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogFile.hpp"
#include "pmdDef.hpp"
#include <vector>
using namespace std;

namespace engine
{

   #define DPS_LOG_FILE_PREFIX "sequoiadbLog."

   class _dpsLogPage;
   class _dpsMessageBlock;
   class _dpsReplicaLogMgr ;

   /*
      _dpsLogFileMgr define
   */
   class _dpsLogFileMgr : public SDBObject
   {
   private:
      vector<_dpsLogFile *>   _files;
      UINT32                  _work;
      UINT32                  _logicalWork ;
      UINT32                  _begin ;
      BOOLEAN                 _rollFlag ;

      UINT32                  _logFileSz ;
      UINT32                  _logFileNum ;
      _dpsReplicaLogMgr       *_replMgr ;

   public:
      _dpsLogFileMgr( class _dpsReplicaLogMgr *replMgr );
      ~_dpsLogFileMgr();

      INT32 init( const CHAR *path );

      INT32 flush( _dpsMessageBlock *mb,
                   const DPS_LSN &beginLsn,
                   BOOLEAN shutdown = FALSE );

      INT32 load( const DPS_LSN &lsn, _dpsMessageBlock *mb,
                  BOOLEAN onlyHeader = FALSE,
                  UINT32 *pLength = NULL ) ;

      INT32 move( const DPS_LSN_OFFSET &offset, const DPS_LSN_VER &version ) ;

      void setLogFileSz ( UINT64 logFileSz )
      {
         _logFileSz = logFileSz ;
      }

      DPS_LSN getStartLSN ( BOOLEAN mustExist = TRUE ) ;

      UINT32 getLogFileSz ()
      {
         return _logFileSz ;
      }
      void setLogFileNum ( UINT32 logFileNum )
      {
         _logFileNum = logFileNum ;
      }
      UINT32 getLogFileNum ()
      {
         return _logFileNum ;
      }
      _dpsLogFile * getWorkLogFile()
      {
         return _files[_work] ;
      }

      _dpsLogFile* getLogFile( UINT32 fileId )
      {
         if ( fileId >= _logFileNum )
         {
            return NULL ;
         }

         return _files[ fileId ] ;
      }

      UINT32 getWorkPos() const { return _work ; }
      UINT32 getLogicalWorkPos() const { return _logicalWork ; }

      INT32 sync() ;

   protected:
      void     _analysis () ;
      UINT32   _incFileID ( UINT32 fileID ) ;
      UINT32   _decFileID ( UINT32 fileID ) ;
      void     _incLogicalFileID () ;

   };
   typedef class _dpsLogFileMgr dpsLogFileMgr ;

}

#endif // DPSLOGFILEMGR_HPP__

