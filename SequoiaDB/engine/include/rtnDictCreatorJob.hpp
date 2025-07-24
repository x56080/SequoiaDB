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

   Source File Name = rtnDictCreatorJob.hpp

   Descriptive Name = Rtn Dictionary Creating Job.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_DICTCREATOR_JOB_HPP_
#define RTN_DICTCREATOR_JOB_HPP_

#include "dmsStorageUnit.hpp"
#include "rtnBackgroundJobBase.hpp"
#include "utilLZWDictionary.hpp"

namespace engine
{
   #define RTN_DEFAULT_DICT_SCAN_INTERVAL ( OSS_ONE_SEC * 3 )

   class _rtnDictCreatorJob : public _rtnBaseJob
   {
   public:
      _rtnDictCreatorJob ( UINT32 scanInterval ) ;
      virtual ~_rtnDictCreatorJob () ;
   public :
      virtual RTN_JOB_TYPE type () const ;
      virtual const CHAR* name() const ;
      virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
      virtual INT32 doit () ;
   private:
      INT32 _checkAndCreateDictForCL( dmsDictJob &job, BOOLEAN &retry ) ;
      BOOLEAN _conditionMatch( dmsStorageUnit *su, dmsMBContext *context,
                               dmsDictJob &job ) ;
      INT32 _createDict( dmsStorageDataCommon *sd, dmsMBContext *context,
                         utilLZWDictCreator *creator ) ;
   private:
      UINT32 _scanInterval ;
   } ;
   typedef _rtnDictCreatorJob rtnDictCreatorJob ;

   INT32 startDictCreatorJob ( EDUID *pEDUID,
                               UINT32 scanInterval = RTN_DEFAULT_DICT_SCAN_INTERVAL ) ;
}

#endif /* RTN_DICT_CREATOR_JOB_HPP_ */

