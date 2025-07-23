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

   Source File Name = dmsLightJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/12/2019  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_LIGHT_JOB_HPP__
#define DMS_LIGHT_JOB_HPP__

#include "utilLightJobBase.hpp"
#include "dms.hpp"

namespace engine
{

   /*
      _dmsDeleteRecordJob define
   */
   class _dmsDeleteRecordJob : public _utilLightJob
   {
      public:
         _dmsDeleteRecordJob( INT32 csID, UINT16 clID,
                              UINT32 csLID, UINT32 clLID,
                              const dmsRecordID &rid ) ;
         virtual ~_dmsDeleteRecordJob() ;

         virtual const CHAR*     name() const ;
         virtual INT32           doit( IExecutor *pExe,
                                       UTIL_LJOB_DO_RESULT &result,
                                       UINT64 &sleepTime ) ;

      protected:
         INT32             _csID ;
         UINT16            _clID ;
         UINT32            _csLID ;
         UINT32            _clLID ;
         dmsRecordID       _rid ;
   } ;
   typedef _dmsDeleteRecordJob dmsDeleteRecordJob ;

   void dmsStartAsyncDeleteRecord( INT32 csID, UINT16 clID,
                                   UINT32 csLID, UINT32 clLID,
                                   const dmsRecordID &rid) ;

}

#endif //DMS_LIGHT_JOB_HPP__

