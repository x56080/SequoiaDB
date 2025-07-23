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

   Source File Name = seAdptIndexSession.hpp

   Descriptive Name = Index session on search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_INDEX_SESSION_HPP_
#define SEADPT_INDEX_SESSION_HPP_

#include "pmdAsyncSession.hpp"
#include "pmdEDU.hpp"
#include "seAdptDBAssist.hpp"
#include "seAdptSEAssist.hpp"
#include "seAdptIndexState.hpp"
#include "seAdptIdxMetaMgr.hpp"

using engine::pmdEDUCB ;

namespace seadapter
{
   // Indexer sessions on search engine adapter. It's a context of the indexing
   // operation.
   class _seAdptIndexSession : public _pmdAsyncSession
   {
   public:
      _seAdptIndexSession( UINT64 sessionID, seIdxMetaContext *idxMeta ) ;
      virtual ~_seAdptIndexSession() ;

      virtual EDU_TYPES eduType() const ;
      virtual SDB_SESSION_TYPE sessionType() const ;
      virtual const CHAR* className() const { return "Indexer" ; }

      // Called by session manager to check if this session times out. If yes,
      // the session will be released.
      virtual BOOLEAN timeout( UINT32 interval ) ;

      // Called by self thread, in the main loop of the thread.
      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

      seIdxMetaContext* idxMetaContext() const ;

      seAdptDBAssist* dbAssist() const ;

      seAdptSEAssist* seAssist() ;

      void triggerStateTransition( SEADPT_INDEXER_STATE targetState ) ;

      UINT64 currentRequestID() const
      {
         return _requestID ;
      }

      UINT64 nextRequestID()
      {
         return ++_requestID ;
      }

      UINT32 tid() const
      {
         return _tid ;
      }

      void setExpectLID( INT64 expectLID )
      {
         _lastExpectLID = _expectLID ;
         _expectLID = expectLID ;
      }

      INT64 getExpectLID() const
      {
         return _expectLID ;
      }

      INT64 getLastExpectLID() const
      {
         return _lastExpectLID ;
      }

      const CHAR *getESIdxName() const
      {
         return _seIdxName ;
      }

      const CHAR *getESTypeName() const
      {
         return _seTypeName ;
      }

   protected:
      virtual void _onAttach() ;
      virtual void _onDetach() ;

      virtual INT32 _defaultMsgFunc( NET_HANDLE handle, MsgHeader *msg );

   private:
      INT32 _init( seAdptDBAssist *dbAssist, UINT32 tid ) ;

      seAdptIndexerState* _getStateInstance( SEADPT_INDEXER_STATE state ) ;

      BOOLEAN _needStateTransition() const ;

      INT32 _stateTransition() ;

      /**
       * @brief Clean obsolete context on data node. Obsolete contexts may be
       * appeared when timeout for query respond. In that case, new query may
       * have been sent, and the old context will leak if not killed manually.
       */
      INT32 _cleanObsoleteContext( NET_HANDLE handle, MsgHeader *msg ) ;

      void _cleanup() ;

   private:
      BOOLEAN _quit ;
      BOOLEAN _initialized ;
      seIdxMetaContext *_imContext ;
      CHAR _seIdxName[ SEADPT_MAX_IDXNAME_SZ + 1 ] ;
      CHAR _seTypeName[ SEADPT_MAX_TYPE_SZ + 1 ] ;

      seAdptIndexerState *_stateInstance ;
      SEADPT_INDEXER_STATE _targetState ;

      // DB assist is hold by the adapter CB. One instance for all.
      seAdptDBAssist *_dbAssist ;
      seAdptSEAssist _seAssist ;
      utilESClt *_searchEngine ;
      UINT32 _tid ;  // Internal TID to fill in messages.
      UINT64 _requestID ;
      INT64 _expectLID ;
      INT64 _lastExpectLID ;
   } ;
   typedef _seAdptIndexSession seAdptIndexSession ;
}

#endif /* SEADPT_INDEX_SESSION_HPP_ */

