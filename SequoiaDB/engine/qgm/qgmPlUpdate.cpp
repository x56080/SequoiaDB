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

   Source File Name = qgmPlUpdate.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "qgmPlUpdate.hpp"
#include "qgmConditionNodeHelper.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "rtnCoordUpdate.hpp"
#include "msgMessage.hpp"
#include "utilStr.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

using namespace bson ;

namespace engine
{
   _qgmPlUpdate::_qgmPlUpdate( const _qgmDbAttr &collection,
                               const BSONObj &modifer,
                               _qgmConditionNode *condition )
   :_qgmPlan( QGM_PLAN_TYPE_UPDATE, _qgmField() ),
    _collection( collection ),
    _updater( modifer )
   {
      try
      {
         if ( NULL != condition )
         {
            _qgmConditionNodeHelper tree( condition ) ;
            _condition = tree.toBson( TRUE ) ;
         }
         _initialized = TRUE ;
      }
      catch ( std::exception &e )
      {
        PD_LOG( PDERROR, "unexcepted err happened:%s", e.what() ) ;
      }
   }

   _qgmPlUpdate::~_qgmPlUpdate()
   {
   }

   BOOLEAN _qgmPlUpdate::needRollback() const
   {
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLUPDATE__EXEC, "_qgmPlUpdate::_execute" )
   INT32 _qgmPlUpdate::_execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLUPDATE__EXEC ) ;
      INT32 rc = SDB_OK ;
      _SDB_KRCB *krcb = pmdGetKRCB() ;
      SDB_ROLE role = krcb->getDBRole() ;
      BSONObj hint ;

      if ( SDB_ROLE_COORD == role )
      {
         rtnCoordUpdate update ;
         CHAR *msg = NULL ;
         INT32 size = 0 ;
         INT64 contextID = -1 ;
         rc = msgBuildUpdateMsg( &msg, &size,
                                 _collection.toString().c_str(),
                                 0, 0,
                                 &_condition,
                                 &_updater,
                                 &hint ) ;

         if ( SDB_OK != rc )
         {
            SDB_OSS_FREE( msg ) ;
            msg = NULL ;
            goto error ;
         }

         rc = update.execute( (MsgHeader*)msg, eduCB, contextID, NULL ) ;
         SDB_OSS_FREE( msg ) ;
         msg = NULL ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
         SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

         if ( dpsCB && eduCB->isFromLocal() && !dpsCB->isLogLocal() )
         {
             dpsCB = NULL ;
         }
         rc = rtnUpdate( _collection.toString().c_str(),
                         _condition,
                         _updater,
                         hint,
                         0, eduCB, dmsCB, dpsCB ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLUPDATE__EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
