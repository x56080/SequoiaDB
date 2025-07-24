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

   Source File Name = dmsCB.hpp

   Descriptive Name = Data Management Service Control Block Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data management control block, which is the metatdata information for DMS
   component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSCB_HPP_
#define DMSCB_HPP_

#include "core.hpp"
#include "dmsMmapEngine.hpp"
#include "interface/IDataManagementService.h"
#include "oss.hpp"
#include "ossMem.hpp"
#include "dms.hpp"
#include "monLatch.hpp"
#include "monDMS.hpp"
#include "dmsTempSUMgr.hpp"
#include "dmsStatSUMgr.hpp"
#include "dmsRBSMgr.hpp"
#include "dmsLocalSUMgr.hpp"
#include "ossAtomic.hpp"
#include "ossRWMutex.hpp"
#include "dpsLogWrapper.hpp"
#include "ossEvent.hpp"
#include "sdbInterface.hpp"
#include "dmsIxmKeySorter.hpp"
#include "dmsStorageJob.hpp"
#include "ossMemPool.hpp"
#include "dmsScanner.hpp"
#include "dmsEngineSocket.hpp"
#include "dmsSuConstraintMap.hpp"

using namespace std;

namespace engine
{
   class _pmdEDUCB;
   class _dmsStorageUnit;

// 20 minutes
#define DMS_DFT_BLOCKWRITE_TIMEOUT ( 20 * 60 * OSS_ONE_SEC )

#define DMS_MAX_CS_NUM 16384
#define DMS_INVALID_CS DMS_INVALID_SUID

/*
   DMS_STATE DEFINE
*/
#define DMS_STATE_NORMAL 0
#define DMS_STATE_READONLY 1
#define DMS_STATE_ONLINE_BACKUP 2
#define DMS_STATE_FULLSYNC 3
#define DMS_STATE_RESTORE 4

/*
   OTHER DEFINE
*/
#define DMS_CHANGESTATE_WAIT_LOOP 100

   /*
      _SDB_DMSCB define
   */
   class _SDB_DMSCB : public _IControlBlock, public IDataManagementService
   {
      private:
         struct cmp_cscb
         {
               bool operator()( const char *a, const char *b )
               {
                  return std::strcmp( a, b ) < 0;
               }
         };

         monSpinXLatch _stateMtx;
         ossEvent _blockEvent;
         SINT64 _writeCounter;
         UINT8 _dmsCBState;

         dmsTempSUMgr _tempSUMgr;
         dmsStatSUMgr _statSUMgr;
         dmsRBSMgr _rbsSUMgr;
         dmsLocalSUMgr _localSUMgr;

         dmsEngineSocket _engineSocket;
         dmsSuConstraintMap _cm;
         dmsMmapEngine *_mmapEngine = nullptr;

      private:
         INT32 _changeIndexUniqueID( _dmsStorageUnit *su,
                                     const ossPoolVector< ossPoolString > &changedClVec,
                                     const ossPoolVector< BSONObj > &idxInfoObj,
                                     pmdEDUCB *cb );

         dmsMmapEngine *_getMmapEngine() const;

      public:
         _SDB_DMSCB();
         virtual ~_SDB_DMSCB();

         virtual SDB_CB_TYPE cbType() const override
         {
            return SDB_CB_DMS;
         }
         virtual const CHAR *cbName() const override
         {
            return "DMSCB";
         }

         virtual INT32 init() override;
         virtual INT32 active() override;
         virtual INT32 deactive() override;
         virtual INT32 fini() override;
         virtual void onConfigChange() override;

         virtual INT32 createCS( IExecutor *executor,
                                 const CHAR *name,
                                 utilCSUniqueID uniqueId,
                                 const dmsCreateCSOptions &o,
                                 const bson::BSONObj &adjunct ) override;

         virtual INT32 dropCS( IExecutor *executor,
                               const CHAR *name,
                               const dmsRemoveCSOptions &options ) override;

         virtual INT32 renameCS( IExecutor *executor,
                                 const CHAR *oldName,
                                 const CHAR *newName,
                                 BOOLEAN blockWrite ) override;

         virtual INT32 openCL( IExecutor *executor,
                               const CHAR *fullName,
                               const dmsOpenCLOptions &o,
                               DATA_COLLECTION_PTR &ptr ) override;

         virtual INT32 openCL( IExecutor *executor,
                               utilCLUniqueID uniqueId,
                               const dmsOpenCLOptions &o,
                               DATA_COLLECTION_PTR &ptr ) override;

         virtual INT32 createCL( IExecutor *executor,
                                 const CHAR *clFullName,
                                 utilCLUniqueID clUniqueID,
                                 const dmsCreateCLOptions &o,
                                 const bson::BSONObj &adjunct ) override;

         virtual INT32 dropCL( IExecutor *executor,
                               const CHAR *clFullName,
                               const dmsRemoveCLOptions &o ) override;

         virtual INT32 nameToSuDescriptor( const CHAR *pName, DMS_SU_DESCRIPTOR &desc ) override;

         virtual UINT32 getNullCSUniqueIDCnt() const override;

         INT32 nameToSUAndLock( const CHAR *pName,
                                dmsStorageUnitID &suID,
                                _dmsStorageUnit **su,
                                OSS_LATCH_MODE lockType = SHARED,
                                INT32 millisec = -1 );
         INT32 idToSUAndLock( utilCSUniqueID csUniqueID,
                              dmsStorageUnitID &suID,
                              _dmsStorageUnit **su,
                              OSS_LATCH_MODE lockType = SHARED,
                              INT32 millisec = -1 );

         INT32 verifySUAndLock( const dmsEventSUItem *pSUItem,
                                _dmsStorageUnit **ppSU,
                                OSS_LATCH_MODE lockType = SHARED,
                                INT32 millisec = -1 );

         _dmsStorageUnit *suLock( dmsStorageUnitID suID );
         void suUnlock( dmsStorageUnitID suID, OSS_LATCH_MODE lockType = SHARED );

         INT32 changeUniqueID( const CHAR *csname,
                               utilCSUniqueID csUniqueID,
                               const BSONObj &clInfoObj,
                               BOOLEAN changeOtherCL,
                               const ossPoolVector< BSONObj > *pIdxInfoVec,
                               BOOLEAN changeIdx,
                               pmdEDUCB *cb,
                               SDB_DPSCB *dpsCB,
                               BOOLEAN isLoadCS = FALSE );

         INT32 addCollectionSpace( const CHAR *pName,
                                   UINT32 topSequence,
                                   _dmsStorageUnit *su,
                                   _pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB,
                                   BOOLEAN isCreate );

         INT32 dropCollectionSpace( const CHAR *pName,
                                    _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB,
                                    dmsDropCSOptions *options = NULL );
         INT32 dropEmptyCollectionSpace( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );

         INT32 dropCollectionSpaceP1( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );

         INT32 dropCollectionSpaceP1Cancel( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );

         INT32 dropCollectionSpaceP2( const CHAR *pName,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB,
                                      dmsDropCSOptions *options = NULL );

         INT32 unloadCollectonSpace( const CHAR *pName, _pmdEDUCB *cb );

         INT32 renameCollectionSpace( const CHAR *pName,
                                      const CHAR *pNewName,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB );
         INT32 renameCollectionSpaceP1( const CHAR *pName,
                                        const CHAR *pNewName,
                                        _pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB );
         INT32 renameCollectionSpaceP1Cancel( const CHAR *pName,
                                              const CHAR *pNewName,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB );
         INT32 renameCollectionSpaceP2( const CHAR *pName,
                                        const CHAR *pNewName,
                                        _pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB );
         INT32 restoreCollectionSpace( const CHAR *pName );
         INT32 returnCollectionSpaceP1( dmsReturnOptions &options,
                                        _pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB );
         INT32 returnCollectionSpaceP1Cancel( dmsReturnOptions &options,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB );
         INT32 returnCollectionSpaceP2( dmsReturnOptions &options,
                                        _pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB );
         INT32 returnCollectionSpace( dmsReturnOptions &options, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );

         INT32 dumpInfo( MON_CL_SIM_LIST &collectionList, BOOLEAN sys = FALSE );
         INT32 dumpInfo( MON_CS_SIM_LIST &csList,
                         BOOLEAN sys = FALSE,
                         BOOLEAN dumpCL = FALSE,
                         BOOLEAN dumpIdx = FALSE );

         INT32 dumpInfo( MON_CL_LIST &collectionList, BOOLEAN sys = FALSE );
         INT32 dumpInfo( MON_CS_LIST &csList, BOOLEAN sys = FALSE );
         INT32 dumpInfo( MON_SU_LIST &storageUnitList, BOOLEAN sys = FALSE );

         void dumpInfo( INT64 &totalFileSize );

         void dumpPageMapCSInfo( MON_CSNAME_VEC &vecCS );

         dmsTempSUMgr *getTempSUMgr();

         dmsStatSUMgr *getStatSUMgr();

         _dmsRBSMgr *getRBSSUMgr();

         dmsLocalSUMgr *getLocalSUMgr();

         void clearSUCaches( UINT32 mask );

         void clearSUCaches( const MON_CS_SIM_LIST &monCSList, UINT32 mask );

         void changeSUCaches( UINT32 mask );

         void changeSUCaches( const MON_CS_SIM_LIST &monCSList, UINT32 mask );

         BOOLEAN dispatchDictJob( dmsDictJob &job );
         void pushDictJob( dmsDictJob job );

         void setIxmKeySorterCreator( dmsIxmKeySorterCreator *creator );
         INT32 createIxmKeySorter( INT64 bufSize,
                                   const _dmsIxmKeyComparer &comparer,
                                   dmsIxmKeySorter **ppSorter );
         void releaseIxmKeySorter( dmsIxmKeySorter *pSorter );

         void setScannerCheckerCreator( IDmsScannerCheckerCreator *pCreator );
         INT32 createScannerChecker( UINT32 suLID,
                                     UINT32 mbLID,
                                     const CHAR *csName,
                                     const CHAR *clShortName,
                                     const CHAR *optrDesc,
                                     _pmdEDUCB *cb,
                                     IDmsScannerChecker **ppChecker );
         void releaseScannerChecker( IDmsScannerChecker *pChecker );

         INT32 getMaxDMSLSN( DPS_LSN_OFFSET &maxLsn );

      public:
         typedef std::vector< SDB_DMS_CSCB * >::iterator CSCB_ITERATOR;
         OSS_INLINE CSCB_ITERATOR begin()
         {
            return _getMmapEngine()->begin();
         }

         OSS_INLINE CSCB_ITERATOR end()
         {
            return _getMmapEngine()->end();
         }

         INT32 writable( _pmdEDUCB *cb );
         void writeDown( _pmdEDUCB *cb );

         INT32 blockWrite( _pmdEDUCB *cb,
                           SDB_DB_STATUS byStatus = SDB_DB_NORMAL,
                           INT32 timeout = DMS_DFT_BLOCKWRITE_TIMEOUT );
         void unblockWrite( _pmdEDUCB *cb );

         INT32 registerBackup( _pmdEDUCB *cb, BOOLEAN offline = TRUE );
         void backupDown( _pmdEDUCB *cb );

         INT32 registerRebuild( _pmdEDUCB *cb );
         void rebuildDown( _pmdEDUCB *cb );

         INT32 registerFullSync( _pmdEDUCB *cb );
         void fullSyncDown( _pmdEDUCB *cb );

         INT32 registerRestore( _pmdEDUCB *cb );
         void restoreDown( _pmdEDUCB *cb );

         UINT8 getCBState() const;

         void clearAllCRUDCB();
         INT32 clearSUCRUDCB( const CHAR *collectionSpace );
         INT32 clearMBCRUDCB( const CHAR *collection );

         INT32 regHandler( DMS_ENGINE_TYPE engineType, _IDmsEventHandler *pHandler );
         void unregHandler( DMS_ENGINE_TYPE engineType, _IDmsEventHandler *pHandler );
   };
   typedef class _SDB_DMSCB SDB_DMSCB;

   /*
      get global SDB_DMSCB
   */
   SDB_DMSCB *sdbGetDMSCB();
} // namespace engine

#endif // DMSCB_HPP_
