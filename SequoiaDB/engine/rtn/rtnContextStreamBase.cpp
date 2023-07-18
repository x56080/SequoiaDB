/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
