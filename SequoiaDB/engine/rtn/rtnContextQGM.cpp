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

   Source File Name = rtnContextQGM.cpp

   Descriptive Name = RunTime QGM Context

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
#include "rtnContextQGM.hpp"
#include "qgmPlanContainer.hpp"
#include "dmsStorageBase.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   /*
      _rtnContextQGM implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextQGM, RTN_CONTEXT_QGM, "QGM")

   _rtnContextQGM::_rtnContextQGM( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID )
   {
      _accPlan    = NULL ;
   }

   _rtnContextQGM::~_rtnContextQGM ()
   {
      if ( NULL != _accPlan )
      {
         SAFE_OSS_DELETE( _accPlan ) ;
         _accPlan = NULL ;
      }
   }

   const CHAR* _rtnContextQGM::name() const
   {
      return "QGM" ;
   }

   RTN_CONTEXT_TYPE _rtnContextQGM::getType () const
   {
      return RTN_CONTEXT_QGM ;
   }

   INT32 _rtnContextQGM::open( qgmPlanContainer *accPlan )
   {
      INT32 rc = SDB_OK ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }
      if ( NULL == accPlan )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _accPlan = accPlan ;
      _isOpened = TRUE ;
      _hitEnd = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextQGM::_prepareData( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      INT32 index = 0 ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      for ( ; index < RTN_CONTEXT_GETNUM_ONCE ; ++index )
      {
         try
         {
            rc = _accPlan->fetch( obj, cb ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Qgm fetch failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = append( obj ) ;
         PD_RC_CHECK( rc, PDERROR, "Append obj[%s] failed, rc: %d",
                      obj.toString().c_str(), rc ) ;

         // increase counter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_SELECT, 1 ) ;

         // make sure we still have room to read another
         // record_max_sz (i.e. 16MB). if we have less than 16MB
         // to 256MB, we can't safely assume the next record we
         // read will not overflow the buffer, so let's just break
         // before reading the next record
         if ( buffEndOffset() + DMS_RECORD_MAX_SZ > RTN_RESULTBUFFER_SIZE_MAX )
         {
            // let's break if there's no room for another max record
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

   RTN_CTX_AUTO_REGISTER(_rtnContextQgmSort, RTN_CONTEXT_QGMSORT, "QGMSORT")

   _rtnContextQgmSort::_rtnContextQgmSort( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _qp(NULL)
   {

   }

   _rtnContextQgmSort::~_rtnContextQgmSort()
   {
     /// qgmPlan should be released by plan tree.
      _qp = NULL ;
   }

   const CHAR* _rtnContextQgmSort::name() const
   {
      return "QGMSORT" ;
   }

   RTN_CONTEXT_TYPE _rtnContextQgmSort::getType () const
   {
      return RTN_CONTEXT_QGMSORT ;
   }

   INT32 _rtnContextQgmSort::open( _qgmPlan *qp )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != qp, "impossible" ) ;
      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }

      _qp = qp ;
      _isOpened = TRUE ;
      _hitEnd = FALSE ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextQgmSort::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != _qp, "impossible" ) ;
      qgmFetchOut next ;
      INT32 index = 0 ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      for ( ; index < RTN_CONTEXT_GETNUM_ONCE ; ++index )
      {
         try
         {
            rc = _qp->fetchNext( next, cb ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Qgm fetch failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = append( next.obj ) ;
         PD_RC_CHECK( rc, PDERROR, "Append obj[%s] failed, rc: %d",
                      next.obj.toString().c_str(), rc ) ;
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

