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
          11/11/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef BAR_RESTORE_JOB_HPP__
#define BAR_RESTORE_JOB_HPP__

#include "rtnBackgroundJob.hpp"
#include "barBkupLogger.hpp"

namespace engine
{

   /*
      _barRestoreJob define
   */
   class _barRestoreJob : public _rtnBaseJob
   {
      public:
         _barRestoreJob ( barRSBaseLogger *pRSLogger ) ;
         virtual ~_barRestoreJob () ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

      private:
         barRSBaseLogger         *_rsLogger ;

   } ;
   typedef _barRestoreJob  barRestoreJob ;

   INT32 startRestoreJob ( EDUID *pEDUID, barRSBaseLogger *pRSLogger ) ;

}

#endif //BAR_RESTORE_JOB_HPP__

