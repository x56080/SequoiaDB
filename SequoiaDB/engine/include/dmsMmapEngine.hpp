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

   Source File Name = dmsMmapEngine.hpp

   Descriptive Name = Data Management Service Mmap Engine Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data management control block, which is the metatdata information for DMS
   component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/29/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_MMAP_ENGINE_HPP_
#define DMS_MMAP_ENGINE_HPP_

#include "interface/IDataManagementService.h"
#include "dms.hpp"
#include "dmsSuConstraintMap.hpp"
#include "dmsStorageJob.hpp"
#include "dmsDictJob.hpp"
#include "dmsScanner.hpp"
#include "dmsIxmKeySorter.hpp"
#include "monDMS.hpp"

using namespace std;

namespace engine
{
   class _pmdEDUCB;
   class _dmsStorageUnit;

// 20 minutes
#define DMS_DFT_BLOCKWRITE_TIMEOUT ( 20 * 60 * OSS_ONE_SEC )

   // for each collection space, there is one CSCB associate with it
   class _SDB_DMS_CSCB : public SDBObject
   {
      public:
         // ossSpinSLatch _mutex ;
         //  maximum sequence id for the collection space
         //  currently 1 sequence per collection space
         UINT32 _topSequence;
         CHAR _name[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ];
         _dmsStorageUnit *_su;

         _SDB_DMS_CSCB( const CHAR *pName, UINT32 topSequence, _dmsStorageUnit *su )
         {
            ossStrncpy( _name, pName, DMS_COLLECTION_SPACE_NAME_SZ );
            _name[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0;
            _topSequence = topSequence;
            _su = su;
         }
         ~_SDB_DMS_CSCB();
   };
   typedef class _SDB_DMS_CSCB SDB_DMS_CSCB;

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
      _dmsMmapEngine define
   */
   class _dmsMmapEngine : public IDataStorageEngine
   {
      private:
         monSpinSLatch _mutex;

         struct cmp_cscb
         {
               bool operator()( const char *a, const char *b )
               {
                  return std::strcmp( a, b ) < 0;
               }
         };
         ossPoolMap< const CHAR *, dmsStorageUnitID, cmp_cscb > _cscbNameMap;
         ossPoolMap< utilCSUniqueID, dmsStorageUnitID > _cscbIDMap;
         std::vector< SDB_DMS_CSCB * > _cscbVec;
         std::vector< SDB_DMS_CSCB * > _tmpCscbVec;
         std::vector< BYTE > _tmpCscbStatusVec;
         std::vector< ossRWMutex * > _latchVec;
         // use deque so we can operate on both ends
         std::queue< dmsStorageUnitID > _freeList;

#if defined( _WINDOWS )
         typedef ossPoolMap< const CHAR *, dmsStorageUnitID, cmp_cscb >::const_iterator
            CSCB_MAP_CONST_ITER;
         typedef ossPoolMap< const CHAR *, dmsStorageUnitID, cmp_cscb >::iterator CSCB_MAP_ITER;
#elif defined( _LINUX )
         typedef ossPoolMap< const CHAR *, dmsStorageUnitID >::const_iterator CSCB_MAP_CONST_ITER;
         typedef ossPoolMap< const CHAR *, dmsStorageUnitID >::iterator CSCB_MAP_ITER;
#endif
         typedef ossPoolMap< utilCSUniqueID, dmsStorageUnitID >::const_iterator
            CSCB_ID_MAP_CONST_ITER;
         typedef ossPoolMap< utilCSUniqueID, dmsStorageUnitID >::iterator CSCB_ID_MAP_ITER;

         /*
          * Queue of collections which are waitting for dictionaies creation.
          * Here we store the storage unit id and mb ID of the collection.
          * One concern here is the reuse of these two IDs, but that's ok. I'll
          * just check with these IDs to see if dictionary creation is needed for
          * the CURRENT corresponding collection. If they have been reused, then
          * the original collection has been dropped. So we just ignore that.
          * If the IDs have been reused, and added to the queue again, it's also
          * ok.
          * Everytime we are about to create a dictionary, we should check if a
          * dictionary is there already. If yes, just remove the item from the
          * queue.
          */
         ossQueue< dmsDictJob > _dictWaitQue;

         UINT32 _logicalSUID;

         dmsIxmKeySorterCreator *_ixmKeySorterCreator;
         IDmsScannerCheckerCreator *_scannerCheckerCreator;
         dmsPageMappingDispatcher _pageMapDispatcher;

         DMS_HANDLER_LIST _handlers;

      private:
         void _logCSCBNameMap();

         INT32 _CSCBNameInsert( const CHAR *pName,
                                UINT32 topSequence,
                                _dmsStorageUnit *su,
                                dmsStorageUnitID &suID );

         INT32 _CSCBNameLookup( const CHAR *pName,
                                SDB_DMS_CSCB **cscb,
                                dmsStorageUnitID *pSuID = NULL,
                                BOOLEAN exceptDeleting = TRUE );
         INT32 _CSCBIdLookup( utilCSUniqueID csUniqueID,
                              SDB_DMS_CSCB **cscb,
                              dmsStorageUnitID *pSuID = NULL,
                              BOOLEAN exceptDeleting = TRUE );
         INT32 _CSCBLookup( const CHAR *pName,
                            utilCSUniqueID csUniqueID,
                            SDB_DMS_CSCB **cscb,
                            dmsStorageUnitID *pSuID = NULL,
                            BOOLEAN exceptDeleting = TRUE );

         INT32 _CSCBNameLookupAndLock( const CHAR *pName,
                                       dmsStorageUnitID &suID,
                                       SDB_DMS_CSCB **cscb,
                                       OSS_LATCH_MODE lockType = SHARED,
                                       INT32 millisec = -1 );
         INT32 _CSCBIdLookupAndLock( utilCSUniqueID csUniqueID,
                                     dmsStorageUnitID &suID,
                                     SDB_DMS_CSCB **cscb,
                                     OSS_LATCH_MODE lockType = SHARED,
                                     INT32 millisec = -1 );

         void _CSCBRelease( dmsStorageUnitID suID, OSS_LATCH_MODE lockType = SHARED );

         INT32 _moveCSCB2TmpList( const CHAR *pName, BYTE status );

         INT32 _restoreCSCBFromTmpList( const CHAR *pName );

         INT32 _CSCBRename( const CHAR *pName,
                            const CHAR *pNewName,
                            _pmdEDUCB *cb,
                            SDB_DPSCB *dpsCB );

         INT32 _CSCBRenameP1( const CHAR *pName,
                              const CHAR *pNewName,
                              _pmdEDUCB *cb,
                              SDB_DPSCB *dpsCB );

         INT32 _CSCBRenameP1Cancel( const CHAR *pName,
                                    const CHAR *pNewName,
                                    _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB );

         INT32 _CSCBRenameP2( const CHAR *pName,
                              const CHAR *pNewName,
                              _pmdEDUCB *cb,
                              SDB_DPSCB *dpsCB );

         INT32 _CSCBNameRemoveP1( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );
         INT32 _CSCBNameRemoveP1Cancel( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );
         INT32 _CSCBNameRemoveP2( const CHAR *pName,
                                  dmsDropCSOptions *options,
                                  _pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB,
                                  SDB_DMS_CSCB *&pCSCB );

         void _CSCBNameMapCleanup();

         INT32 _delCollectionSpace( const CHAR *pName,
                                    _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN removeFile,
                                    BOOLEAN onlyEmpty,
                                    dmsDropCSOptions *options = NULL );

         INT32 _delCollectionSpaceP1( const CHAR *pName,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB,
                                      BOOLEAN removeFile = TRUE );

         INT32 _delCollectionSpaceP1Cancel( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );

         INT32 _delCollectionSpaceP2( const CHAR *pName,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB,
                                      BOOLEAN removeFile = TRUE,
                                      dmsDropCSOptions *options = NULL );

         INT32 _getCSList( ossPoolVector< ossPoolString > &csNameVec );

         INT32 _changeIndexUniqueID( _dmsStorageUnit *su,
                                     const ossPoolVector< ossPoolString > &changedClVec,
                                     const ossPoolVector< BSONObj > &idxInfoObj,
                                     pmdEDUCB *cb );

         INT32 _loadCollectionSpaces( const CHAR *dataPath,
                                      const CHAR *indexPath,
                                      const CHAR *lobPath,
                                      const CHAR *lobMetaPath,
                                      dmsSuConstraintMap &cm );

         INT32 _resumeClDictCreate( const CHAR *csName );

      public:
         _dmsMmapEngine();
         virtual ~_dmsMmapEngine();

         virtual INT32 close( IExecutor *executor, const dmsCloseDBOptions &options ) override;

         virtual DMS_ENGINE_TYPE getEngineType() const override
         {
            return DMS_ENGINE_MMAP;
         }

         virtual INT32 createCS( IExecutor *executor,
                                 const CHAR *name,
                                 const utilCSUniqueID &uniqueId,
                                 const dmsCreateCSOptions &o,
                                 const bson::BSONObj &adjunct,
                                 DMS_SU_DESCRIPTOR &desc ) override;

         virtual INT32 testCS( IExecutor *executor,
                               const CHAR *name,
                               utilCSUniqueID &uniqueId ) override;

         virtual INT32 testCS( IExecutor *executor, utilCSUniqueID uniqueId ) override;

         virtual INT32 listCS( IExecutor *executor, DATA_CURSOR_PTR &cursor ) override;

         virtual INT32 getCSCount( IExecutor *executor, UINT32 &countt ) override;

         virtual INT32 removeCS( IExecutor *executor,
                                 const CHAR *name,
                                 const dmsRemoveCSOptions &delOptions ) override;

         virtual INT32 removeEmptyCS( IExecutor *executor,
                                      const CHAR *name,
                                      const dmsRemoveCSOptions &delOptions ) override;

         virtual INT32 renameCS( IExecutor *executor,
                                 const CHAR *name,
                                 const CHAR *newName,
                                 BOOLEAN blockWrite,
                                 DMS_SU_DESCRIPTOR &desc ) override;

         virtual INT32 unloadCS( IExecutor *executor,
                                 const CHAR *name,
                                 const dmsRemoveCSOptions &delOptions ) override;

         virtual INT32 restoreCS( IExecutor *executor, const CHAR *name ) override;

         virtual INT32 returnCS( IExecutor *executor,
                                 dmsReturnOptions &options,
                                 BOOLEAN blockWrite ) override;

         virtual INT32 nameToSuDescriptor( const CHAR *pName, DMS_SU_DESCRIPTOR &desc ) override;

         virtual INT32 createCL( IExecutor *executor,
                                 const CHAR *fullName,
                                 utilCLUniqueID uniqueId,
                                 const dmsCreateCLOptions &o,
                                 const bson::BSONObj &adjunct ) override;

         virtual INT32 removeCL( IExecutor *executor,
                                 const CHAR *fullName,
                                 const dmsRemoveCLOptions &o ) override;

         virtual INT32 testCL( IExecutor *executor,
                               const CHAR *fullName,
                               utilCLUniqueID &uniqueId ) override;

         virtual INT32 testCL( IExecutor *executor, utilCLUniqueID uniqueId ) override;

         virtual INT32 listCL( IExecutor *executor,
                               const CHAR *csName,
                               DATA_CURSOR_PTR &cursor ) override;

         virtual INT32 openCL( IExecutor *executor,
                               const CHAR *fullName,
                               const dmsOpenCLOptions &o,
                               DATA_COLLECTION_PTR &ptr ) override;

         virtual INT32 openCL( IExecutor *executor,
                               utilCLUniqueID uniqueId,
                               const dmsOpenCLOptions &o,
                               DATA_COLLECTION_PTR &ptr ) override;

         virtual INT32 getCLCount( IExecutor *executor,
                                   const CHAR *csName,
                                   UINT32 &count ) override;

      public:
         // temporary
         virtual INT32 createCS( IExecutor *executor,
                                 const CHAR *name,
                                 const utilCSUniqueID &uniqueId,
                                 const dmsCreateCSOptions &o,
                                 const bson::BSONObj &adjunct ) override;

         virtual INT32 removeCS( IExecutor *executor, const CHAR *csName ) override;

      public:
         INT32 open( dmsSuConstraintMap &cm );

         INT32 findCollectionSpace( const CHAR *pName, dmsStorageUnitID &suId );

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

         INT32 changeCSUniqueID( _dmsStorageUnit *su, utilCSUniqueID csUniqueID );

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
                                   BOOLEAN isCreate,
                                   DMS_SU_DESCRIPTOR &desc );

         INT32 delCollectionSpace( IExecutor *executor,
                                   const CHAR *pCollectionSpace,
                                   IDataProtectionService *dps,
                                   BOOLEAN sysCall,
                                   BOOLEAN dropFile,
                                   BOOLEAN ensureEmpty,
                                   dmsDropCSOptions *options = nullptr );

         INT32 dropCollectionSpace( const CHAR *pName,
                                    _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB,
                                    dmsDropCSOptions *options = NULL );

         INT32 dropEmptyCollectionSpace( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );

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

         void clearSUCaches( UINT32 mask );

         void clearSUCaches( const MON_CS_SIM_LIST &monCSList, UINT32 mask );

         void changeSUCaches( UINT32 mask );

         void changeSUCaches( const MON_CS_SIM_LIST &monCSList, UINT32 mask );

         INT32 dropCollectionSpaceP1( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );

         INT32 dropCollectionSpaceP1Cancel( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB );

         INT32 dropCollectionSpaceP2( const CHAR *pName,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB,
                                      dmsDropCSOptions *options = NULL );

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
            return _cscbVec.begin();
         }

         OSS_INLINE CSCB_ITERATOR end()
         {
            return _cscbVec.end();
         }

         void clearAllCRUDCB();
         INT32 clearSUCRUDCB( const CHAR *collectionSpace );
         INT32 clearMBCRUDCB( const CHAR *collection );

         INT32 regHandler( _IDmsEventHandler *pHandler );
         void unregHandler( _IDmsEventHandler *pHandler );

         void onConfigChange();
   };
   typedef class _dmsMmapEngine dmsMmapEngine;
} // namespace engine

#endif // DMSCB_HPP_
