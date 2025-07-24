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

   Source File Name = catGTSMsgJob.hpp

   Descriptive Name = GTS message job

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_GTS_MSG_JOB_HPP_
#define CAT_GTS_MSG_JOB_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "rtnBackgroundJobBase.hpp"

namespace engine
{
   class _catGTSMsgHandler ;

   class _catGTSMsgJob: public _rtnBaseJob
   {
   public:
      _catGTSMsgJob( _catGTSMsgHandler* msgHandler, BOOLEAN isController, INT32 timeout ) ;
      virtual ~_catGTSMsgJob() ;

      OSS_INLINE BOOLEAN isController() const { return _isController ; }

   public:
      virtual RTN_JOB_TYPE type() const ;
      virtual const CHAR* name() const ;
      virtual BOOLEAN muteXOn( const _rtnBaseJob *pOther ) ;
      virtual INT32 doit() ;
      virtual BOOLEAN reuseEDU() const { return TRUE ; }

   private:
      _catGTSMsgHandler*   _msgHandler ;
      BOOLEAN              _isController ;
      INT32                _timeout ;
   } ;
   typedef _catGTSMsgJob catGTSMsgJob ;

   INT32 catStartGTSMsgJob( _catGTSMsgHandler* msgHandler,
                            BOOLEAN isController,
                            INT32 timeout ) ;
}

#endif /* CAT_GTS_MSG_JOB_HPP_ */

