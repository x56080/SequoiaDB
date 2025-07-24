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

   Source File Name = rtnPageCleanerJob.hpp

   Descriptive Name = Page cleaner header. Page cleaner is a type of background
                      job that flush storage unit in period of time.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/04/2014  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_PAGECLEANER_JOB_HPP_
#define RTN_PAGECLEANER_JOB_HPP_

#include "rtnBackgroundJob.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   /*
    * _rtnPageCleanerJob define
    */
   class _rtnPageCleanerJob : public _rtnBaseJob
   {
   public :
      _rtnPageCleanerJob ( INT32 periodTime = OSS_ONE_SEC ) ;
      virtual ~_rtnPageCleanerJob () ;
   public :
      virtual RTN_JOB_TYPE type () const ;
      virtual const CHAR* name() const ;
      virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
      virtual INT32 doit () ;

   private:
      void _tryToSyncDB() ;
   private :
      INT32   _periodTime ;
      UINT64  _lastTick ;
   } ;
   typedef _rtnPageCleanerJob rtnPageCleanerJob ;

   INT32 startPageCleanerJob ( EDUID *pEDUID,
                               INT32 periodTime = OSS_ONE_SEC ) ;
}

#endif // RTN_PAGECLEANER_JOB_HPP_
