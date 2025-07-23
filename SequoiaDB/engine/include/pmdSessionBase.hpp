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

   Source File Name = pmdSessionBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/12/2020  LYB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_SESSION_BASE_HPP_
#define PMD_SESSION_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sdbInterface.hpp"
#include "pmdIProcessor.hpp"
#include "pmdOperator.hpp"

namespace engine
{

   class _pmdEDUCB ;
   class _dpsLogWrapper ;
   class _pmdProcessorBase ;

   /*
      _pmdSessionBase define
   */
   class _pmdSessionBase : public _ISession
   {
   public:
      _pmdSessionBase() ;
      virtual ~_pmdSessionBase() ;

      void     setFirstMsgTime( UINT64 time ) { _firstMsgTime = time ; }
      UINT64   getFirstMsgTime() const { return _firstMsgTime ; }

   public:
      virtual _pmdEDUCB*      eduCB () const = 0 ;
      virtual EDUID           eduID () const = 0 ;

      virtual BOOLEAN         isBusinessSession() const { return FALSE ; }
      virtual IProcessor*     getProcessor() ;
      virtual _dpsLogWrapper* getDPSCB() { return NULL ; }
      virtual void            attachProcessor( _pmdProcessorBase *pProcessor ) ;
      virtual void            detachProcessor() ;
      virtual IOperator*      getOperator() ;

      virtual INT32 checkPrivilegesForCmd( const CHAR *cmdName,
                                           const CHAR *pQuery,
                                           const CHAR *pSelector,
                                           const CHAR *pOrderby,
                                           const CHAR *pHint );

      virtual INT32 checkPrivilegesForActionsOnExact( const CHAR *pCollectionName,
                                                      const authActionSet &actions );

      virtual INT32 checkPrivilegesForActionsOnCluster( const authActionSet &actions );

      virtual INT32 checkPrivilegesForActionsOnResource( const boost::shared_ptr< authResource > &,
                                                         const authActionSet &actions );

      virtual BOOLEAN privilegeCheckEnabled();

      virtual INT32 getACL( boost::shared_ptr<const authAccessControlList> &acl );

   protected:
      _pmdProcessorBase *_processor ;
      UINT64            _firstMsgTime ;
   } ;

   typedef _pmdSessionBase pmdSessionBase ;

   class _pmdProcessorBase : public _IProcessor
   {
   friend class _pmdSessionBase ;
   public:
      _pmdProcessorBase() ;
      virtual ~_pmdProcessorBase() ;

      virtual ISession* getSession() { return _pSession ; }

   protected:
      virtual void      _attachSession( pmdSessionBase *pSession ) ;
      virtual void      _detachSession() ;

      _dpsLogWrapper*   getDPSCB() ;
      _IClient*         getClient() ;
      _pmdEDUCB*        eduCB() ;
      EDUID             eduID() const ;

   protected:
      pmdSessionBase*   _pSession ;
   } ;
   typedef _pmdProcessorBase pmdProcessorBase ;
}

#endif //PMD_SESSION_BASE_HPP_

