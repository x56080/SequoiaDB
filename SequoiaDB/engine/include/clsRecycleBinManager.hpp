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

   Source File Name = clsRecycleBinManager.hpp

   Descriptive Name = Recycle Bin Manager Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_RECYCLE_BIN_MGR_HPP__
#define CLS_RECYCLE_BIN_MGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pmdEDU.hpp"
#include "rtnRecycleBinManager.hpp"
#include "clsShardMgr.hpp"
#include "dmsLocalSUMgr.hpp"
#include "rtnLocalTaskMgr.hpp"

namespace engine
{

   class _clsMgr ;

   /*
      _clsRecycleBinManager define
    */
   class _clsRecycleBinManager : public _rtnRecycleBinManager,
                                 public _IDmsEventHandler
   {
   protected:
         typedef _rtnRecycleBinManager _BASE ;

   public:
      _clsRecycleBinManager() ;
      virtual ~_clsRecycleBinManager() ;

      INT32 init() ;
      void fini() ;

      INT32 dropItemWithCheck( const utilRecycleItem &item,
                               pmdEDUCB *cb,
                               BOOLEAN checkCatalog,
                               BOOLEAN &isDropped ) ;
      INT32 dropAllItems( pmdEDUCB *cb, BOOLEAN checkCatalog ) ;

      INT32 getSubItems( const utilRecycleItem &item,
                         pmdEDUCB *cb,
                         UTIL_RECY_ITEM_LIST &itemList ) ;

      INT32 saveItem( const utilRecycleItem &item, pmdEDUCB *cb ) ;

      INT32 getItem( const CHAR *recycleName,
                     pmdEDUCB *cb,
                     utilRecycleItem &item ) ;

      INT32 deleteItem( const utilRecycleItem &item, pmdEDUCB *cb )
      {
         return _BASE::_deleteItem( item, cb, 1, TRUE ) ;
      }

      INT32 deleteItemsInCS( utilCSUniqueID csUniqueID,
                             pmdEDUCB *cb )
      {
         return _deleteItemsInCS( csUniqueID, TRUE, cb, 1 ) ;
      }

      // dmsEventHandler implements
      OSS_INLINE virtual UINT32 getMask () const
      {
         return DMS_EVENT_MASK_RECY ;
      }

      OSS_INLINE virtual const CHAR *getName() const
      {
         return "recycle bin manager" ;
      }

      // truncate collection callbacks
      virtual INT32 onCheckTruncCL( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    const dmsEventCLItem &clItem,
                                    dmsTruncCLOptions *options,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

      virtual INT32 onTruncateCL( SDB_EVENT_OCCUR_TYPE type,
                                  IDmsEventHolder *pEventHolder,
                                  IDmsSUCacheHolder *pCacheHolder,
                                  const dmsEventCLItem &clItem,
                                  dmsTruncCLOptions *options,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

      virtual INT32 onCleanTruncCL( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    const dmsEventCLItem &clItem,
                                    dmsTruncCLOptions *options,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

      // drop collection callbacks
      virtual INT32 onCheckDropCL( IDmsEventHolder *pEventHolder,
                                   IDmsSUCacheHolder *pCacheHolder,
                                   const dmsEventCLItem &clItem,
                                   dmsDropCLOptions *options,
                                   pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB ) ;

      virtual INT32 onDropCL( SDB_EVENT_OCCUR_TYPE type,
                              IDmsEventHolder *pEventHolder,
                              IDmsSUCacheHolder *pCacheHolder,
                              const dmsEventCLItem &clItem,
                              dmsDropCLOptions *options,
                              pmdEDUCB *cb,
                              SDB_DPSCB *dpsCB ) ;

      virtual INT32 onCleanDropCL( IDmsEventHolder *pEventHolder,
                                   IDmsSUCacheHolder *pCacheHolder,
                                   const dmsEventCLItem &clItem,
                                   dmsDropCLOptions *options,
                                   pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB ) ;

      // drop collection space callbacks
      virtual INT32 onCheckDropCS( IDmsEventHolder *pEventHolder,
                                   IDmsSUCacheHolder *pCacheHolder,
                                   const dmsEventSUItem &suItem,
                                   dmsDropCSOptions *options,
                                   pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB ) ;

      virtual INT32 onDropCS( SDB_EVENT_OCCUR_TYPE type,
                              IDmsEventHolder *pEventHolder,
                              IDmsSUCacheHolder *pCacheHolder,
                              const dmsEventSUItem &suItem,
                              dmsDropCSOptions *options,
                              pmdEDUCB *cb,
                              SDB_DPSCB *dpsCB ) ;

      virtual INT32 onCleanDropCS( IDmsEventHolder *pEventHolder,
                                   IDmsSUCacheHolder *pCacheHolder,
                                   const dmsEventSUItem &suItem,
                                   dmsDropCSOptions *options,
                                   pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB ) ;

   protected:
      virtual const CHAR *_getRecyItemCL() const
      {
         return DMS_SYSLOCALRECYCLEITEM_CL_NAME ;
      }

      virtual ossXLatch *_getDropLatch()
      {
         return NULL ;
      }

      virtual INT32 _dropItem( const utilRecycleItem &item,
                               pmdEDUCB *cb,
                               INT16 w,
                               BOOLEAN ignoreNotExists,
                               BOOLEAN &isDropped ) ;

      virtual INT32 _createBGJob( utilLightJob **pJob ) ;

      INT32 _dropItemImpl( const utilRecycleItem &item,
                           pmdEDUCB *cb,
                           INT16 w ) ;

      INT32 _dropAllItemInType( UTIL_RECYCLE_TYPE type,
                                pmdEDUCB *cb,
                                BOOLEAN checkCatalog ) ;
      INT32 _dropCSItem( const utilRecycleItem &item,
                         pmdEDUCB *cb,
                         INT16 w ) ;
      INT32 _dropMainCLItem( const utilRecycleItem &item,
                             pmdEDUCB *cb,
                             INT16 w ) ;
      INT32 _dropCLItem( const utilRecycleItem &item,
                         pmdEDUCB *cb,
                         INT16 w ) ;

      INT32 _deleteItemsInCS( utilCSUniqueID csUniqueID,
                              BOOLEAN includeSelf,
                              pmdEDUCB *cb,
                              INT16 w ) ;

      INT32 _recycleDropCL( const CHAR *clShortName,
                            dmsStorageUnit *su,
                            dmsMBContext *mbContext,
                            const utilRecycleItem &item,
                            pmdEDUCB *cb,
                            BOOLEAN needChageUniqueID ) ;

      INT32 _recycleTruncCL( const CHAR *clShortName,
                             dmsStorageUnit *su,
                             dmsMBContext *mbContext,
                             const utilRecycleItem &item,
                             pmdEDUCB *cb ) ;

      INT32 _recycleDropCS( dmsStorageUnit *su,
                            const CHAR *csName,
                            const utilRecycleItem &item,
                            pmdEDUCB *cb,
                            BOOLEAN needLogRename ) ;

      INT32 _regBlockCL( const CHAR *originName,
                         const CHAR *recycleName,
                         const dmsEventCLItem &clItem,
                         UINT64 &opID,
                         pmdEDUCB *cb ) ;
      void _unregBlockCL( const CHAR *originName,
                          const CHAR *recycleName,
                          UINT64 opID ) ;

      INT32 _regBlockCS( const CHAR *originName,
                         const CHAR *recycleName,
                         const dmsEventSUItem &suItem,
                         UINT64 &opID,
                         pmdEDUCB *cb ) ;
      void _unregBlockCS( const CHAR *originName,
                          const CHAR *recycleName,
                          UINT64 opID ) ;

      INT32 _createRecycleCLTask( const CHAR *originName,
                                  const CHAR *recycleName,
                                  const utilRecycleItem &item,
                                  pmdEDUCB *cb,
                                  UINT64 &taskID ) ;

      INT32 _createRecycleCSTask( const CHAR *originName,
                                  const CHAR *recycleName,
                                  const utilRecycleItem &item,
                                  pmdEDUCB *cb,
                                  UINT64 &taskID ) ;

      void _cleanLocalTask( UINT64 taskID,
                            UINT64 &opID,
                            pmdEDUCB *cb ) ;

   protected:
      clsFreezingWindow *  _freezingWindow ;
      rtnLocalTaskMgr *    _localTaskManager ;
   } ;

   typedef class _clsRecycleBinManager clsRecycleBinManager ;

   /*
      _clsDropRecycleBinBGJob define
    */
   class _clsDropRecycleBinBGJob : public _rtnDropRecycleBinBGJob
   {
   public:
      _clsDropRecycleBinBGJob( rtnRecycleBinManager *recycleBinMgr ) ;
      virtual ~_clsDropRecycleBinBGJob() ;

      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;
   } ;

   typedef class _clsDropRecycleBinBGJob clsDropRecycleBinBGJob ;

}

#endif // CLS_RECYCLE_BIN_MGR_HPP__
