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

   Source File Name = qgmPlanContainer.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "qgmPlanContainer.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   _qgmPlanContainer::_qgmPlanContainer()
   :_plan( NULL )
   {

   }

   _qgmPlanContainer::~_qgmPlanContainer()
   {
      if ( NULL != _plan )
      {
         _plan->close() ;
         SDB_OSS_DEL _plan ;
         _plan = NULL ;
      }
   }

   INT32 _qgmPlanContainer::execute( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( NULL == _plan )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "can not execute a empty plan" ) ;
         goto error ;
      }
#if defined (_DEBUG)
      {
      INT32 bufferSize = 1024*1024*10 ;
      CHAR *pBuffer = (CHAR*)SDB_OSS_MALLOC(bufferSize) ;
      if ( pBuffer )
      {
         rc = qgmDump ( this, pBuffer, bufferSize ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG ( PDEVENT, "Plan:"OSS_NEWLINE"%s", pBuffer ) ;
         }
         SDB_OSS_FREE ( pBuffer ) ;
      }
      }
#endif

      rc = _plan->execute( cb ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _qgmPlanContainer::needRollback() const
   {
      return _plan ? _plan->needRollback() : FALSE ;
   }

   INT32 _qgmPlanContainer::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      qgmFetchOut next ;

      if ( NULL == _plan )
      {
         PD_LOG( PDERROR, "can not fetch a empty plan" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _plan->fetchNext( next ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      obj = next.obj ;
   done:
      return rc ;
   error:
      goto done ;
   }
}
