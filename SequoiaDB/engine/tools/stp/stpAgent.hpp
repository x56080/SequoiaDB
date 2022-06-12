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

   Source File Name = stpAgent.hpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef STP_AGENT_HPP_
#define STP_AGENT_HPP_

#include "stpCommon.hpp"
#include "stpLogicalTime.hpp"
#include "stpMetaData.hpp"
#include "stpClient.hpp"
#include "utilSHMBuffer.hpp"
#include "utilNodeOpr.hpp"

namespace engine
{

   /*
      _stpAgent define
    */
   // _stpAgent gets logical time from shared memory
   // NOTE: currently, STP agent used inside SequoiaDB
   class _stpAgent : public utilPooledObject
   {
   public:
      // constructor and destructor
      _stpAgent() ;
      ~_stpAgent() ;

   public:
      // quick check if STP is available
      BOOLEAN isAvailable() ;

      // check if STP is available
      INT32 checkAvailable() ;

      // notify the STP to synchronize with server
      INT32 notifySync( INT64 timeout = STP_NOTIFY_DFT_TIMEOUT ) ;

      // get logical time in nanosecond in given timeout
      // output:
      // - time: global logical time returned from STP
      // input:
      // - timeout: timeout to get global logical time
      // - monotonic: indicate if monotonic time is required
      // output:
      // - pWaitedTime: return total wait time
      // return:
      // - SDB_OK: succeed to get global logical time
      // - other return code: failed to get global logical time
      // NOTE: `timeout` is -1 means never timeout
      //       `timeout` is 0 means only try once
      INT32 getLogicalTimeNS( stpLogicalTimeNS &time,
                              INT32 timeout = -1,
                              BOOLEAN monotonic = TRUE,
                              INT32 *pWaitedTime = NULL ) ;

      // try to get logical time in microseconds in given timeout
      // output:
      // - time: global logical time returned from STP
      // input:
      // - timeout: timeout to get global logical time
      // - monotonic: indicate if monotonic time is required
      // output:
      // - pWaitedTime: return total wait time
      // return:
      // - SDB_OK: succeed to get global logical time
      // - other return code: failed to get global logical time
      // NOTE: `timeout` is -1 means never timeout
      //       `timeout` is 0 means only try once
      INT32 getLogicalTimeUS( stpLogicalTimeUS &time,
                              INT32 timeout = -1,
                              BOOLEAN monotonic = TRUE,
                              INT32 *pWaitedTime = NULL ) ;

      // try to get logical time in nanosecond
      // output:
      // - time: global logical time returned from STP
      // input:
      // - monotonic: indicate if monotonic time is required
      // return:
      // - SDB_OK: succeed to get global logical time
      // - other return code: failed to get global logical time
      OSS_INLINE INT32 tryGetLogicalTimeNS( stpLogicalTimeNS &time,
                                            BOOLEAN monotonic = TRUE )
      {
         return getLogicalTimeNS( time, 0, monotonic ) ;
      }

      // try to get logical time in microseconds
      // output:
      // - time: global logical time returned from STP
      // input:
      // - monotonic: indicate if monotonic time is required
      // return:
      // - SDB_OK: succeed to get global logical time
      // - other return code: failed to get global logical time
      OSS_INLINE INT32 tryGetLogicalTimeUS( stpLogicalTimeUS &time,
                                            BOOLEAN monotonic = TRUE )
      {
         return getLogicalTimeUS( time, 0, monotonic ) ;
      }

      // get interface of STP client
      INT32 getClient( stpClient &client ) ;
   } ;

   typedef class _stpAgent stpAgent ;

}

#endif // STP_AGENT_HPP_
