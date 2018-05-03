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

   Source File Name = rtnPrefetchJob.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/09/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_PREFETCH_JOB_HPP_
#define RTN_PREFETCH_JOB_HPP_

#include "rtnBackgroundJob.hpp"

#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{

   /*
      _rtnPrefetchJob define
   */
   class _rtnPrefetchJob : public _rtnBaseJob
   {
      public:
         _rtnPrefetchJob ( INT32 timeout = -1 ) ;
         virtual ~_rtnPrefetchJob () ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      private:
         INT32    _timeout ;

   } ;
   typedef _rtnPrefetchJob  rtnPrefetchJob ;

   INT32 startPrefetchJob ( EDUID *pEDUID, INT32 timeout = -1 ) ;

}

#endif //RTN_PREFETCH_JOB_HPP_

