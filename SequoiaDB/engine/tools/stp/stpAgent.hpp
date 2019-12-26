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
#include "utilSHMBuffer.hpp"
#include "utilNodeOpr.hpp"

namespace engine
{

   /*
      _stpAgent define
    */
   // _stpAgent gets logical time from shared memory
   // NOTE: currently, STP agent used inside SequoiaDB
   class _stpAgent : public utilPooledObject,
                     public stpMetaReader
   {
   public:
      // constructor and destructor
      _stpAgent() ;
      ~_stpAgent() ;

   public:
      // activate STP agent
      INT32 active( BOOLEAN mustAvailable ) ;
      // deactivate STP agent
      INT32 deactive() ;

   public:
      // quick check if STP is available
      OSS_INLINE BOOLEAN isAvailble() const
      {
         return _available ;
      }

      // get logical time in nanosecond in given timeout
      INT32 getLogicalTimeNS( stpLogicalTimeNS &time, INT32 timeout = -1 ) ;
      // try to get logical time in microseconds in given timeout
      INT32 getLogicalTimeUS( stpLogicalTimeUS &time, INT32 timeout = -1 ) ;
      // try to get logical time in nanosecond
      INT32 tryGetLogicalTimeNS( stpLogicalTimeNS &time ) ;
      // try to get logical time in microseconds
      INT32 tryGetLogicalTimeUS( stpLogicalTimeUS &time ) ;

      // check if STP is available
      INT32 checkAvailable() ;

   protected:
      // clear agent
      void  _clear() ;
      // get STP node by checking PID
      INT32 _getSTP() ;
      // test alive of STP node
      INT32 _testSTP() ;
      // check and attach meta data
      INT32 _checkMetaData( const CHAR *shmKey ) ;
      // release meta data
      INT32 _releaseMetaData() ;
      // get logical time in nanoseconds
      INT32 _getLogicalTimeNS( stpLogicalTimeNS &time ) ;

      // attach shared memory buffer
      INT32 _attachSHMBuffer( const CHAR *shmKey ) ;
      // release shared memory buffer
      INT32 _releaseSHMBuffer() ;
      // re-check if we could retry to get logical time
      BOOLEAN _recheckAvailable( INT32 rc ) ;

   protected:
      // PID of STP
      OSSPID            _stpPID ;
      // service name ( port ) of STP
      ossPoolString     _stpServiceName ;
      // lock to protect meta data from shared memory
      // - when reads meta data, should get the shared lock
      // - when attaches or releases meta data, should get the exclusive lock
      ossRWMutex        _metaMutex ;
      // synchronize interval extracted from meta data
      volatile UINT32   _syncInterval ;
      // indicate STP is available
      volatile BOOLEAN  _available ;
      // shared memory buffer
      utilSHMBuffer     _buffer ;
   } ;

   typedef class _stpAgent stpAgent ;

}

#endif // STP_AGENT_HPP_
