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

   Source File Name = pmdOperator.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Operaotr Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for EDU Control
   Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          11/15/2022  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDOPERATOR_HPP__
#define PMDOPERATOR_HPP__

#include "sdbInterface.hpp"
#include "ossUtil.hpp"
#include "dpsDef.hpp"
#include "pmdEnv.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      _pmdOperator define
   */
   class _pmdOperator : public _IOperator
   {
   public:
      _pmdOperator()
      {
         _pMsg = NULL ;
         _maxTime = -1 ;
         _beginTick = 0 ;
         _hasInterruptOnTimeLimit = FALSE ;
         _isContextDetachMode = FALSE ;
         _isContextBatchLimited = FALSE ;
      }
      virtual ~_pmdOperator()
      {
         _pMsg = NULL ;
      }

   public:
      virtual const MsgHeader* getMsg() const
      {
         return _pMsg ;
      }
      virtual const MsgGlobalID& getGlobalID() const
      {
         return _globalID ;
      }
      virtual void updateGlobalID( const MsgGlobalID &globalID )
      {
         _globalID = globalID ;
         if ( _pMsg )
         {
            _pMsg->globalID = _globalID ;
         }
      }
      /*
         <0 means no limit
      */
      virtual INT64 getRemainingMaxTime() const
      {
         if ( _maxTime < 0 )
         {
            return -1 ;
         }

         UINT64 timeSpent = pmdGetTickSpanTime( _beginTick ) ;
         if ( (UINT64)_maxTime > timeSpent )
         {
            return _maxTime - timeSpent ;
         }
         return 0 ;
      }
      virtual INT64 getMaxTime() const { return _maxTime ; }
      virtual void  setMaxTime( INT64 maxTime )
      {
         _maxTime = maxTime ;
         _beginTick = pmdGetDBTick() ;
      }
      virtual BOOLEAN needInterrupt() const
      {
         if ( !_hasInterruptOnTimeLimit && 0 == getRemainingMaxTime() )
         {
            _hasInterruptOnTimeLimit = TRUE ;
         }
         return _hasInterruptOnTimeLimit ;
      }
      virtual BOOLEAN isInterruptOnTimeLimit() const
      {
         return _hasInterruptOnTimeLimit ;
      }

      virtual BOOLEAN isContextDetachMode() const
      {
         return _isContextDetachMode ;
      }

      virtual BOOLEAN isContextBatchLimited() const
      {
         return _isContextBatchLimited ;
      }

   public:
      void setMsg( MsgHeader *pMsg, IExecutor *cb )
      {
         if ( pMsg )
         {
            _pMsg = pMsg ;
            _globalID = _pMsg->globalID ;

            if ( cb )
            {
               /// in transaction, can't use detach mode context
               if ( DPS_INVALID_TRANS_ID != cb->getTransID() ||
                    NULL == cb->getSession() ||
                    ( SDB_SESSION_LOCAL != cb->getSession()->sessionType() &&
                      SDB_SESSION_SHARD != cb->getSession()->sessionType() &&
                      SDB_SESSION_PROTOCOL != cb->getSession()->sessionType() ) )
               {
                  OSS_BIT_CLEAR( _pMsg->flags, FLAG_DETACH_CONTEXT ) ;
                  _isContextDetachMode = FALSE ;
               }
               else if ( _isContextDetachMode )
               {
                  OSS_BIT_SET( _pMsg->flags, FLAG_DETACH_CONTEXT ) ;
               }
               else if ( OSS_BIT_TEST( _pMsg->flags, FLAG_DETACH_CONTEXT ) )
               {
                  _isContextDetachMode = TRUE ;
               }
            }
         }
      }
      void reset()
      {
         _pMsg = NULL ;
         _maxTime = -1 ;
         _beginTick = 0 ;
         _hasInterruptOnTimeLimit = FALSE ;
         _isContextDetachMode = FALSE ;
         _isContextBatchLimited = FALSE ;
      }
      void enableContextDetachMode( IExecutor *cb )
      {
         /// not in transaction
         if ( cb && DPS_INVALID_TRANS_ID == cb->getTransID() &&
              NULL != cb->getSession() &&
              ( SDB_SESSION_LOCAL == cb->getSession()->sessionType() ||
                SDB_SESSION_SHARD == cb->getSession()->sessionType() ||
                SDB_SESSION_PROTOCOL == cb->getSession()->sessionType() ) )
         {
            _isContextDetachMode = TRUE ;
            if ( _pMsg )
            {
               OSS_BIT_SET( _pMsg->flags, FLAG_DETACH_CONTEXT ) ;
            }
         }
      }
      void disableContextDetachMode( IExecutor *cb )
      {
         _isContextDetachMode = FALSE ;
         if ( _pMsg )
         {
            OSS_BIT_CLEAR( _pMsg->flags, FLAG_DETACH_CONTEXT ) ;
         }
      }

      void enableContextBatchSizeLimited()
      {
         _isContextBatchLimited = TRUE ;
      }

      void disableContextBatchSizeLimited()
      {
         _isContextBatchLimited = FALSE ;
      }

   private:
      MsgHeader*  _pMsg ;
      MsgGlobalID _globalID ;
      INT64       _maxTime ;     /// ms
      UINT64      _beginTick ;
      mutable BOOLEAN     _hasInterruptOnTimeLimit ;
      BOOLEAN     _isContextDetachMode ;
      BOOLEAN     _isContextBatchLimited ;
   } ;
   typedef _pmdOperator pmdOperator ;

}

#endif // PMDOPERATOR_HPP__