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

   Source File Name = tpAgent.hpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef TP_AGENT_HPP_
#define TP_AGENT_HPP_

#include "tpCommon.hpp"
#include "tpLogicalTime.hpp"
#include "tpMetaData.hpp"
#include "utilSHMBuffer.hpp"
#include "utilNodeOpr.hpp"
#include "rtnBackgroundJobBase.hpp"

namespace engine
{

   /*
      _tpAgent define
    */
   class _tpAgent : public tpMetaReader
   {
   public:
      _tpAgent() ;
      ~_tpAgent() ;

   public:
      INT32 active() ;
      INT32 deactive() ;

   public:
      INT32 getLogicalTimeNS( tpLogicalTimeNS &time ) ;
      INT32 getLogicalTimeUS( tpLogicalTimeUS &time ) ;
      INT32 checkAvailable() ;
      void  waitCheckEvent() ;
      void  clear() ;

   protected:
      OSS_INLINE UINT32 _getCheckInterval()
      {
         return _available ? ( _syncInterval * OSS_ONE_SEC ) : OSS_ONE_SEC ;
      }

      OSS_INLINE void _signalCheck()
      {
         _checkEvent.signal() ;
      }

      INT32 _getTPNode() ;
      INT32 _testTPNode() ;
      INT32 _checkMetaData( const CHAR *shmKey ) ;
      INT32 _releaseMetaData() ;
      INT32 _getLogicalTimeNS( tpLogicalTimeNS &time ) ;
      INT32 _getLogicalTimeUS( tpLogicalTimeUS &time ) ;
      INT32 _waitUpdateEvent( INT64 timeout ) ;

      INT32 _attachSHMBuffer( const CHAR *shmKey ) ;
      INT32 _releaseSHMBuffer() ;

   protected:
      EDUID                _checkJobEDUID ;
      ossEvent             _checkEvent ;
      OSSPID               _tpPID ;
      ossPoolString        _tpServiceName ;
      ossRWMutex           _metaMutex ;
      UINT32               _metaVersion ;
      volatile UINT32      _syncInterval ;
      volatile BOOLEAN     _available ;
      utilSHMBuffer        _buffer ;
   } ;

   typedef class _tpAgent tpAgent ;

   /*
      _tpAgentCheckJob define
    */
   class _tpAgentCheckJob : public rtnBaseJob
   {
   public:
      _tpAgentCheckJob( tpAgent *agent ) ;
      virtual ~_tpAgentCheckJob() ;

   public:
      OSS_INLINE virtual RTN_JOB_TYPE type() const
      {
         return RTN_JOB_TP_AGENT_CHECK ;
      }

      OSS_INLINE virtual const CHAR *name() const
      {
         return "TPAgentCheck" ;
      }

      OSS_INLINE virtual BOOLEAN muteXOn( const rtnBaseJob *other )
      {
         return ( type() == other->type() ) ;
      }

      virtual INT32 doit() ;

   protected:
      tpAgent * _agent ;
   } ;

   typedef class _tpAgentCheckJob tpAgentCheckJob ;

   /*
      helper functions
    */
   INT32 tpAgentStartCheckJob( tpAgent *agent, EDUID *eduID ) ;

}

#endif // TP_AGENT_HPP_
