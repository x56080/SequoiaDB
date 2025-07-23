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

   Source File Name = rtnContextStreamBase.cpp

   Descriptive Name = RunTime Stream Base Context Source

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnContextStreamBase.hpp"
#include "rtn.hpp"
#include "rtnCB.hpp"
#include "rtnStreamSource.hpp"
#include "rtnTrace.hpp"
#include "ossMemPool.hpp"
#include "utilStreamToken.hpp"

namespace engine
{

   /*
      _rtnContextStreamBase implement
    */
   _rtnContextStreamBase::_rtnContextStreamBase( utilStreamType streamType,
                                                 INT64 contextID,
                                                 UINT64 eduID,
                                                 monStreamSourceMonitor &sourceMonitor )
   : _rtnContextBase( contextID, eduID ),
     _rtnStreamSourceProcessorBase(),
     _monitor( streamType, eduID, contextID, sourceMonitor )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXSTREAMBASE_PROCESSCONTROLRECORD, "_rtnContextStreamBase::processControlRecord" )
   INT32 _rtnContextStreamBase::processControlRecord( const BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCTXSTREAMBASE_PROCESSCONTROLRECORD ) ;

      // append record to buffer and update monitor
      rc = append( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append control record, rc: %d", rc ) ;

      _monitor.onControlRecord() ;

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXSTREAMBASE_PROCESSCONTROLRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXSTREAMBASE_PROCESSCHANGERECORD, "_rtnContextStreamBase::processChangeRecord" )
   INT32 _rtnContextStreamBase::processChangeRecord( const BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCTXSTREAMBASE_PROCESSCHANGERECORD ) ;

      // append record to buffer and update monitor
      rc = append( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append change record, rc: %d", rc ) ;

      _monitor.onChangeRecord() ;

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXSTREAMBASE_PROCESSCHANGERECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXSTREAMBASE_PROCESSDATARECORD, "_rtnContextStreamBase::processDataRecord" )
   INT32 _rtnContextStreamBase::processDataRecord( const BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCTXSTREAMBASE_PROCESSDATARECORD ) ;

      // append record to buffer and update monitor
      rc = append( result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append data record, rc: %d", rc ) ;

      _monitor.onDataRecord() ;

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXSTREAMBASE_PROCESSDATARECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
