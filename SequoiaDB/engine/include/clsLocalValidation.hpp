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

   Source File Name = clsLocalValidation.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/03/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_LOCALVALIDATION_HPP_
#define CLS_LOCALVALIDATION_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "ossUtil.hpp"
#include "ossMemPool.hpp"
#include "pmd.hpp"

namespace engine
{
   struct _clsDiskWriteCostTime
   {
      UINT64 beginTime ;
      UINT64 endTime ;
      UINT64 curSpentTime ;
      UINT64 lastSpentTime ;
      INT32  returnCode ;
      BOOLEAN finishAllDiskWrite ;

      _clsDiskWriteCostTime()
      {
         beginTime = 0 ;
         endTime = 0 ;
         curSpentTime = 0 ;
         lastSpentTime = 0 ;
         returnCode = SDB_OK ;
         finishAllDiskWrite = FALSE ;
      }

      _clsDiskWriteCostTime& operator= ( const _clsDiskWriteCostTime &rhs )
      {
         beginTime = rhs.beginTime ;
         endTime = rhs.endTime ;
         curSpentTime = rhs.curSpentTime ;
         lastSpentTime = rhs.lastSpentTime ;
         returnCode = rhs.returnCode ;
         finishAllDiskWrite = rhs.finishAllDiskWrite ;
         return *this ;
      }

      BOOLEAN isValid()
      {
         return ( beginTime != 0 ) ;
      }

      BOOLEAN isFinish()
      {
         return ( beginTime != 0 && endTime != 0 && endTime >= beginTime && curSpentTime != 0 ) ;
      }
   } ;
   typedef _clsDiskWriteCostTime clsDiskWriteCostTime ;

   extern clsDiskWriteCostTime curDiskWriteCostTime ;

   OSS_INLINE clsDiskWriteCostTime& getCurDiskWriteCostTime()
   {
      return curDiskWriteCostTime ;
   }

   class _clsDiskDetector : public SDBObject
   {
   public:
      _clsDiskDetector() ;
      ~_clsDiskDetector() ;

      INT32   detect() ;
      INT32   init() ;

   private:
      INT32   _addFilePath( const CHAR* pFilePath ) ;
      INT32   _tryToWriteFile( const CHAR* pFilePath, clsDiskWriteCostTime &time ) ;
      BOOLEAN _isNeedToDetect() ;

   private:
      ossPoolSet<ossPoolString> _filePathsSet ;
      UINT64 _lastTick ;
      BOOLEAN _isMonitoredRole ;
      BOOLEAN _hasInit ;
      CHAR    *_content ;
   } ;

   class _clsLocalValidation : public SDBObject
   {
   public:
      INT32 run() ;

   private:
      _clsDiskDetector _diskDetector ;
   } ;
}

#endif

