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

   Source File Name = tpCB.hpp

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

#ifndef TP_CB_HPP__
#define TP_CB_HPP__

#include "tpCBCommon.hpp"
#include "msgTp.hpp"
#include "tpOptions.hpp"
#include "tpMsgHandler.hpp"
#include "tpServiceManager.hpp"
#include "tpCatalogManager.hpp"
#include "tpMetaManager.hpp"
#include "tpSourceManager.hpp"
#include "tpSyncManager.hpp"
#include "tpReplManager.hpp"
#include "tpLogicalTime.hpp"
#include "ossMemPool.hpp"
#include "netRouteAgent.hpp"
#include "pmdPipeManager.hpp"
#include "pmdObjBase.hpp"

namespace engine
{

   /*
      _tpCB define
    */
   class _tpCB : public _pmdObjBase,
                  public IControlBlock
   {
   public:
      _tpCB() ;
      virtual ~_tpCB() ;

   public:
      OSS_INLINE virtual SDB_CB_TYPE cbType() const
      {
         return SDB_CB_TP ;
      }

      OSS_INLINE virtual const CHAR *cbName() const
      {
         return "TPCB" ;
      }

      virtual INT32 init() ;
      virtual INT32 active() ;
      virtual INT32 deactive() ;
      virtual INT32 fini() ;

      virtual void  onConfigChange() ;

   public:
      OSS_INLINE tpOptions *getOptions()
      {
         return ( &_options ) ;
      }

      OSS_INLINE const tpOptions *getOptions() const
      {
         return ( &_options ) ;
      }

      OSS_INLINE netRouteAgent *getNetAgent()
      {
         return ( &_netAgent ) ;
      }

      OSS_INLINE pmdPipeManager *getPipeManager()
      {
         return ( &_pipeManager ) ;
      }

      OSS_INLINE tpNetMsgHandler *getNetMsgHandler()
      {
         return ( &_netMsgHandler ) ;
      }

      OSS_INLINE tpPipeMsgHandler *getPipeMsgHandler()
      {
         return ( &_pipeMsgHandler ) ;
      }

      OSS_INLINE tpServiceManager *getServiceManager()
      {
         return ( &_serviceManager ) ;
      }

      OSS_INLINE tpCatalogManager *getCatalogManager()
      {
         return ( &_catalogManager ) ;
      }

      OSS_INLINE tpMetaManager *getMetaManager()
      {
         return ( &_metaManager ) ;
      }

      OSS_INLINE tpSourceManager *getSourceManager()
      {
         return ( &_sourceManager ) ;
      }

      OSS_INLINE tpSyncManager *getSyncManager()
      {
         return ( &_syncManager ) ;
      }

      OSS_INLINE tpReplManager *getReplManager()
      {
         return ( &_replManager ) ;
      }

      OSS_INLINE tpMetaData *getMetaData()
      {
         return _metaManager.getMetaData() ;
      }

      OSS_INLINE BOOLEAN isPrimaryServer()
      {
         return _catalogManager.isPrimaryServer() ;
      }

      OSS_INLINE BOOLEAN isPrimary()
      {
         return _catalogManager.isPrimary() ;
      }

      OSS_INLINE BOOLEAN isSecondaryServer()
      {
         return _catalogManager.isSecondaryServer() ;
      }

      INT32 onChangePrimary( const MsgRouteID &primaryRID,
                             BOOLEAN isLocalPrimary ) ;
      INT32 onChangeServers() ;
      INT32 onChangeRole( TP_ROLE role ) ;

   protected:
      INT32 _registerModule( tpModule *module ) ;
      INT32 _initializeModules() ;

   protected:
      void _checkTimeExInfo() ;
      INT32 _initNetAgent() ;
      INT32 _initPipeManager() ;
      INT32 _activeSystemClock() ;
      INT32 _activeNetAgent() ;
      INT32 _activePipeManager() ;

   protected:
      tpOptions            _options ;
      netRouteAgent        _netAgent ;
      pmdPipeManager       _pipeManager ;
      tpNetMsgHandler      _netMsgHandler ;
      tpPipeMsgHandler     _pipeMsgHandler ;
      tpServiceManager     _serviceManager ;
      tpCatalogManager     _catalogManager ;
      tpMetaManager        _metaManager ;
      tpSourceManager      _sourceManager ;
      tpSyncManager        _syncManager ;
      tpReplManager        _replManager ;
      TP_MODULE_LIST       _moduleList ;
   } ;

   SDB_TPCB *sdbGetTPCB() ;

}

#endif // TP_CB_HPP__
