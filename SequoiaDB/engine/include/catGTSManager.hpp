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

   Source File Name = catGTSManager.hpp

   Descriptive Name = GTS(Global Transaction Service) manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/13/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_GTS_MANAGER_HPP_
#define CAT_GTS_MANAGER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "netDef.hpp"
#include "msg.h"
#include "catGTSMsgHandler.hpp"
#include "catSequenceManager.hpp"
#include "catEventHandler.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class sdbCatalogueCB ;

   class _catGTSManager: public SDBObject,
                         public _catEventHandler
   {
   private:
      // disallow copy and assign
      _catGTSManager( const _catGTSManager& ) ;
      void operator=( const _catGTSManager& ) ;
   public:
      _catGTSManager() ;
      ~_catGTSManager() ;

      INT32 init() ;
      INT32 fini() ;

      void  attachCB( _pmdEDUCB* cb ) ;
      void  detachCB( _pmdEDUCB* cb ) ;

      INT32 active() ;
      INT32 deactive() ;

      INT32 handleMsg( const NET_HANDLE& handle, const MsgHeader* msg ) ;

      virtual const CHAR *getHandlerName() { return "catGTSManager" ; }
      virtual INT32 onUpgrade( UINT32 version ) ;

   public:
      OSS_INLINE _catSequenceManager* getSequenceMgr()
      {
         return &_seqMgr ;
      }

   private:
      INT32 _ensureMetadata() ;
      INT32 _createSysIndex ( const CHAR* clFullName,
                              const CHAR* indexJson,
                              _pmdEDUCB* cb ) ;
      INT32 _createSysCollection ( const CHAR* clFullName,
                                   _pmdEDUCB* cb ) ;

      // add collection unique ID to sequence
      // upgrade from 3.6 / 5.0.3
      INT32 _checkAndUpgradeSequenceCLUID() ;

   private:
      _SDB_DMSCB*          _dmsCB ;
      _pmdEDUCB*           _eduCB ;
      sdbCatalogueCB*      _catCB ;
      _catGTSMsgHandler    _msgHandler ;
      _catSequenceManager  _seqMgr ;
   } ;
   typedef _catGTSManager catGTSManager ;
}

#endif /* CAT_GTS_MANAGER_HPP_ */
