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

   Source File Name = rtnContextSP.cpp

   Descriptive Name = RunTime Store Procedure Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.cpp

   Last Changed =

*******************************************************************************/
#include "rtnContextSP.hpp"
#include "dmsStorageBase.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   /*
      _rtnContextSP define
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextSP, RTN_CONTEXT_SP, "SP")

   _rtnContextSP::_rtnContextSP( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _sp(NULL)
   {

   }

   _rtnContextSP::~_rtnContextSP()
   {
      SAFE_OSS_DELETE( _sp ) ;
   }

   const CHAR* _rtnContextSP::name() const
   {
      return "SP" ;
   }

   RTN_CONTEXT_TYPE _rtnContextSP::getType() const
   {
      return RTN_CONTEXT_SP ;
   }

   INT32 _rtnContextSP::open( _spdSession *sp )
   {
      INT32 rc = SDB_OK ;
      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }
      if ( NULL == sp )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _sp = sp ;
      _isOpened = TRUE ;
      _hitEnd = FALSE ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32  _rtnContextSP::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      for ( INT32 i = 0; i < RTN_CONTEXT_GETNUM_ONCE; i++ )
      {
         rc = _sp->next( obj ) ;
         if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to fetch spdSession:%d", rc ) ;
            goto error ;
         }
         else
         {
            rc = append( obj ) ;
            PD_RC_CHECK( rc, PDERROR, "Append obj[%s] failed, rc: %d",
                      obj.toString().c_str(), rc ) ;
         }

         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_SELECT, 1 ) ;

         if ( buffEndOffset() + DMS_RECORD_MAX_SZ > RTN_RESULTBUFFER_SIZE_MAX )
         {
            break ;
         }
      }

      if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}

