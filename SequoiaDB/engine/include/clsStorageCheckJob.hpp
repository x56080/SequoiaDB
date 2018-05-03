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

   Source File Name = clsStorageCheckJob.hpp

   Descriptive Name = Storage Checking Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef CLS_STORAGE_CHECK_JOB_HPP__
#define CLS_STORAGE_CHECK_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"

namespace engine
{
   // One hour interval
   #define STORAGE_CHECK_UNIT_INTERVAL ( OSS_ONE_SEC * 60 * 60 )

   /*
    *  _clsStorageCheckJob define
    */
   class _clsStorageCheckJob : public _rtnBaseJob
   {
      public:
      _clsStorageCheckJob () ;
      virtual ~_clsStorageCheckJob () ;

   public:
      virtual RTN_JOB_TYPE type () const { return RTN_JOB_CLS_STORAGE_CHECK ; }

      virtual const CHAR* name () const { return "DmsCheck" ; }

      virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) { return FALSE ; }

      virtual INT32 doit () ;
   } ;

   typedef _clsStorageCheckJob clsStorageCheckJob ;

   INT32 startStorageCheckJob ( EDUID *pEDUID ) ;

}

#endif //CLS_STORAGE_CHECK_JOB_HPP__

