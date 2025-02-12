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
#include "oss.hpp"
#include "ossMem.hpp"
#include "dms.hpp"
#include "monLatch.hpp"
#include "monDMS.hpp"
#include "dmsTempSUMgr.hpp"
#include "dmsStatSUMgr.hpp"
#include "dmsRBSSUMgr.hpp"
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

using namespace std ;

namespace engine
{
   class _pmdEDUCB ;
   class _dmsStorageUnit ;

   // 20 minutes
   #define DMS_DFT_BLOCKWRITE_TIMEOUT       ( 20 * 60 * OSS_ONE_SEC )

   // for each collection space, there is one CSCB associate with it
   class _SDB_DMS_CSCB : public SDBObject
   {
   public:
      //ossSpinSLatch _mutex ;
      // maximum sequence id for the collection space
      // currently 1 sequence per collection space
      UINT32 _topSequence ;
      CHAR   _name [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      _dmsStorageUnit *_su ;
      _SDB_DMS_CSCB ( const CHAR *pName, UINT32 topSequence,
                      _dmsStorageUnit *su )
      {
         ossStrncpy ( _name, pName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
         _name[DMS_COLLECTION_SPACE_NAME_SZ] = 0 ;
         _topSequence = topSequence ;
         _su = su ;
      }
      ~_SDB_DMS_CSCB () ;
   } ;
   typedef class _SDB_DMS_CSCB SDB_DMS_CSCB ;


   #define DMS_MAX_CS_NUM 16384
   #define DMS_INVALID_CS DMS_INVALID_SUID

   /*
      DMS_STATE DEFINE
   */
   #define DMS_STATE_NORMAL            0
   #define DMS_STATE_READONLY          1
   #define DMS_STATE_ONLINE_BACKUP     2
   #define DMS_STATE_FULLSYNC          3

   /*
      OTHER DEFINE
   */
   #define DMS_CHANGESTATE_WAIT_LOOP   100

   /*
      _dmsDictJob define
   */
   struct _dmsDictJob
   {
      dmsStorageUnitID _suID ;
      UINT32 _suLID ;
      UINT16 _clID ;
      UINT32 _clLID ;
      UINT64 _recordNum ;
      UINT64 _lastWriteTick ;

      _dmsDictJob()
      : _suID( DMS_INVALID_SUID ),
        _suLID( DMS_INVALID_SUID ),
        _clID( DMS_INVALID_CLID ),
        _clLID( DMS_INVALID_CLID ),
        _recordNum( 0 ),
        _lastWriteTick( 0 )
      {
      }

      _dmsDictJob( dmsStorageUnitID suID, UINT32 suLID, UINT16 clID,
                   UINT32 clLID )
      : _suID( suID ),
        _suLID( suLID ),
        _clID( clID ),
        _clLID( clLID ),
        _recordNum( 0 ),
        _lastWriteTick( 0 )
      {
      }
   } ;
   typedef _dmsDictJob dmsDictJob ;

   #define DMS_CHECK_CS             1
   #define DMS_CHECK_CL             2
   #define DMS_CHECK_EMPTYCS        3
   /*
      _dmsCheckItem define
   */
   struct _dmsCheckItem
   {
      CHAR           _name[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      utilCSUniqueID _csUniqueID ;
      utilCLUniqueID _clUniqueID ;
      UINT32         _type ;
      UINT64         _dbTick ;

      _dmsCheckItem( const CHAR *name = "",
                     const utilCSUniqueID &csUniqueID = UTIL_UNIQUEID_NULL,
                     UINT32 type = 0 )
      {
         ossMemset( _name, 0, sizeof( _name ) ) ;
         setItem( name, csUniqueID, type ) ;
      }

      _dmsCheckItem( const CHAR *name,
                     const utilCLUniqueID &cLUniqueID,
                     UINT32 type )
      {
         ossMemset( _name, 0, sizeof( _name ) ) ;
         setItem( name, cLUniqueID, type ) ;
      }

      void setItem( const CHAR *name, const utilCSUniqueID &csUniqueID, UINT32 type )
      {
         ossStrncpy( _name, name, DMS_COLLECTION_FULL_NAME_SZ ) ;
         _name[ DMS_COLLECTION_FULL_NAME_SZ ] = 0 ;
         _csUniqueID = csUniqueID ;
         _clUniqueID = UTIL_UNIQUEID_NULL ;
         _type = type ;
         _dbTick = pmdGetDBTick() ;
      }

      void setItem( const CHAR *name, const utilCLUniqueID &clUniqueID, UINT32 type )
      {
         ossStrncpy( _name, name, DMS_COLLECTION_FULL_NAME_SZ ) ;
         _name[ DMS_COLLECTION_FULL_NAME_SZ ] = 0 ;
         _csUniqueID = utilGetCSUniqueID( clUniqueID ) ;
         _clUniqueID = clUniqueID ;
         _type = type ;
         _dbTick = pmdGetDBTick() ;
      }

   } ;
   typedef _dmsCheckItem dmsCheckItem ;

   /*
      _dmsDataStatMgr define
   */
   class _dmsDataStatMgr : public _IDataStatManager
   {
   public :
      _dmsDataStatMgr()
      : _pageAllocate( 0 ),
        _pageRelease( 0 )
      {
      }

      ~_dmsDataStatMgr()
      {
      }
   public :
      virtual void incPageAllocate( UINT16 numPages ) { _pageAllocate += numPages ; }
      virtual void incPageRelease( UINT16 numPages ) { _pageRelease += numPages ; }
      virtual UINT64 getPageAllocate() const { return _pageAllocate ; }
      virtual UINT64 getPageRelease() const { return _pageRelease ; }
   private :
      UINT64 _pageAllocate ;
      UINT64 _pageRelease ;
   } ;
   typedef _dmsDataStatMgr dmsDataStatMgr ;

   /*
      _dmsFilter define
   */
   class _dmsFilter : public SDBObject
   {
      public:
         _dmsFilter() {}
         virtual ~_dmsFilter() {}

      public:
         virtual BOOLEAN filter( _dmsStorageUnit *su ) = 0 ;
   } ;
   typedef _dmsFilter dmsFilter ;

   /*
      _dmsEmptyCSFilter define
   */
   class _dmsEmptyCSFilter : public _dmsFilter
   {
      public:
         _dmsEmptyCSFilter() {}
         virtual ~_dmsEmptyCSFilter() {}

      public:
         virtual BOOLEAN filter( _dmsStorageUnit *su ) ;
   } ;
   typedef _dmsEmptyCSFilter dmsEmptyCSFilter ;

   /*
      _dmsNoAccessCSFilter define
   */
   class _dmsNoAccessCSFilter : public _dmsFilter
   {
      public:
         _dmsNoAccessCSFilter( UINT64 noAccessMS ) ;
         virtual ~_dmsNoAccessCSFilter() {}

      public:
         virtual BOOLEAN filter( _dmsStorageUnit *su ) ;

      private:
         UINT64         _noAccessMS ;
   } ;
   typedef _dmsNoAccessCSFilter dmsNoAccessCSFilter ;

   /*
      _SDB_DMSCB define
   */
   class _SDB_DMSCB : public _IControlBlock
   {
   private :
      monSpinSLatch _mutex ;

      struct cmp_cscb
      {
         bool operator() (const char *a, const char *b)
         {
            return std::strcmp(a,b)<0 ;
         }
      } ;
      ossPoolMap<const CHAR*, dmsStorageUnitID, cmp_cscb> _cscbNameMap ;
      ossPoolMap<utilCSUniqueID, dmsStorageUnitID>  _cscbIDMap ;
      std::vector<SDB_DMS_CSCB*>          _cscbVec ;
      std::vector<SDB_DMS_CSCB*>          _tmpCscbVec ;
      std::vector<BYTE>                   _tmpCscbStatusVec ;
      std::vector<ossRWMutex*>            _latchVec ;
      // use deque so we can operate on both ends
      std::queue<dmsStorageUnitID>        _freeList ;
      // collection spaces mutex in create and drop operations
      std::vector< ossSpinRecursiveXLatch* >  _vecCSMutex ;

#if defined (_WINDOWS)
      typedef ossPoolMap<const CHAR*,
                       dmsStorageUnitID,
                       cmp_cscb>::const_iterator CSCB_MAP_CONST_ITER ;
      typedef ossPoolMap<const CHAR*,
                       dmsStorageUnitID,
                       cmp_cscb>::iterator CSCB_MAP_ITER ;
#elif defined (_LINUX)
      typedef ossPoolMap<const CHAR*,
                       dmsStorageUnitID>::const_iterator CSCB_MAP_CONST_ITER ;
      typedef ossPoolMap<const CHAR*,
                       dmsStorageUnitID>::iterator CSCB_MAP_ITER ;
#endif
      typedef ossPoolMap<utilCSUniqueID,
                       dmsStorageUnitID>::const_iterator CSCB_ID_MAP_CONST_ITER ;
      typedef ossPoolMap<utilCSUniqueID,
                       dmsStorageUnitID>::iterator CSCB_ID_MAP_ITER ;

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
      ossQueue<dmsDictJob>    _dictWaitQue ;
      ossQueue<dmsCheckItem>  _checkItemQue ;

      monSpinXLatch           _stateMtx;
      ossEvent                _blockEvent ;
      SINT64                  _writeCounter;
      UINT8                   _dmsCBState;
      UINT32                  _logicalSUID ;

      // how many cs which unique id = 0, except system cs
      UINT32                  _nullCSUniqueIDCnt ;

      dmsTempSUMgr            _tempSUMgr ;
      dmsStatSUMgr            _statSUMgr ;
      dmsLocalSUMgr           _localSUMgr ;
      dmsDataStatMgr          _statMgr ;

      dmsIxmKeySorterCreator* _ixmKeySorterCreator ;
      IDmsScannerCheckerCreator *_scannerCheckerCreator ;
      dmsPageMappingDispatcher   _pageMapDispatcher ;

      DMS_HANDLER_LIST           _handlers ;

   private:
      void  _logCSCBNameMap () ;

      INT32 _CSCBNameInsert ( const CHAR *pName,
                              UINT32 topSequence,
                              _dmsStorageUnit *su,
                              dmsStorageUnitID &suID ) ;

      INT32 _CSCBNameLookup ( const CHAR *pName,
                              SDB_DMS_CSCB **cscb,
                              dmsStorageUnitID *pSuID = NULL,
                              BOOLEAN exceptDeleting = TRUE ) ;
      INT32 _CSCBIdLookup ( utilCSUniqueID csUniqueID,
                            SDB_DMS_CSCB **cscb,
                            dmsStorageUnitID *pSuID = NULL,
                            BOOLEAN exceptDeleting = TRUE ) ;
      INT32 _CSCBLookup ( const CHAR *pName,
                          utilCSUniqueID csUniqueID,
                          SDB_DMS_CSCB **cscb,
                          dmsStorageUnitID *pSuID = NULL,
                          BOOLEAN exceptDeleting = TRUE ) ;

      INT32 _CSCBNameLookupAndLock ( const CHAR *pName,
                                     dmsStorageUnitID &suID,
                                     SDB_DMS_CSCB **cscb,
                                     OSS_LATCH_MODE lockType = SHARED,
                                     INT32 millisec = -1 ) ;
      INT32 _CSCBIdLookupAndLock ( utilCSUniqueID csUniqueID,
                                   dmsStorageUnitID &suID,
                                   SDB_DMS_CSCB **cscb,
                                   OSS_LATCH_MODE lockType = SHARED,
                                   INT32 millisec = -1 ) ;

      void _CSCBRelease ( dmsStorageUnitID suID,
                          OSS_LATCH_MODE lockType = SHARED ) ;

      INT32 _moveCSCB2TmpList( const CHAR *pName, BYTE status ) ;

      INT32 _restoreCSCBFromTmpList( const CHAR *pName ) ;

      INT32 _CSCBRename( const CHAR *pName,
                         const CHAR *pNewName,
                         _pmdEDUCB *cb,
                         SDB_DPSCB *dpsCB ) ;

      INT32 _CSCBRenameP1( const CHAR *pName,
                           const CHAR *pNewName,
                           _pmdEDUCB *cb,
                           SDB_DPSCB *dpsCB ) ;

      INT32 _CSCBRenameP1Cancel( const CHAR *pName,
                                     const CHAR *pNewName,
                                     _pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;

      INT32 _CSCBRenameP2( const CHAR *pName,
                           const CHAR *pNewName,
                           _pmdEDUCB *cb,
                           SDB_DPSCB *dpsCB ) ;

      INT32 _CSCBNameRemoveP1 ( const CHAR *pName,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpsCB ) ;
      INT32 _CSCBNameRemoveP1Cancel ( const CHAR *pName,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) ;
      INT32 _CSCBNameRemoveP2 ( const CHAR *pName,
                                dmsDropCSOptions *options,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpsCB,
                                SDB_DMS_CSCB *&pCSCB ) ;

      void _CSCBNameMapCleanup () ;

      INT32 _delCollectionSpace ( const CHAR *pName, _pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB, BOOLEAN removeFile,
                                  BOOLEAN onlyEmpty,
                                  dmsDropCSOptions *options = NULL,
                                  const CHAR *pExceptShortCLName = NULL ) ;

      INT32 _delCollectionSpaceP1 ( const CHAR *pName, _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN removeFile = TRUE ) ;

      INT32 _delCollectionSpaceP1Cancel ( const CHAR *pName, _pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB );

      INT32 _delCollectionSpaceP2 ( const CHAR *pName, _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN removeFile = TRUE,
                                    dmsDropCSOptions *options = NULL ) ;

      INT32 _getCSList( ossPoolVector< ossPoolString > &csNameVec ) ;

      void _nullCSUniqueIDCntInc() ;

      void _nullCSUniqueIDCntDec() ;

      INT32 _changeIndexUniqueID( _dmsStorageUnit* su,
                                  const ossPoolVector<ossPoolString>& changedClVec,
                                  const ossPoolVector<BSONObj>& idxInfoObj,
                                  pmdEDUCB* cb ) ;

   public:
      _SDB_DMSCB() ;
      virtual ~_SDB_DMSCB() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_DMS ; }
      virtual const CHAR* cbName() const { return "DMSCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;
      virtual void   onConfigChange() ;

      INT32 nameToSUAndLock ( const CHAR *pName,
                              dmsStorageUnitID &suID,
                              _dmsStorageUnit **su,
                              OSS_LATCH_MODE lockType = SHARED,
                              INT32 millisec = -1 ) ;
      INT32 idToSUAndLock ( utilCSUniqueID csUniqueID,
                            dmsStorageUnitID &suID,
                            _dmsStorageUnit **su,
                            OSS_LATCH_MODE lockType = SHARED,
                            INT32 millisec = -1 ) ;

      INT32 verifySUAndLock ( const dmsEventSUItem *pSUItem,
                              _dmsStorageUnit **ppSU,
                              OSS_LATCH_MODE lockType = SHARED,
                              INT32 millisec = -1 ) ;

      INT32 nameToSULID( const CHAR *pName,
                         UINT32 &suLogicalID ) ;
      INT32 nameToCSInfo( const CHAR *pName,
                          dmsStorageUnitID &suID,
                          UINT32 &csLID,
                          utilCSUniqueID &csUniqueID ) ;

      _dmsStorageUnit *suLock ( dmsStorageUnitID suID ) ;
      void suUnlock ( dmsStorageUnitID suID,
                      OSS_LATCH_MODE lockType = SHARED ) ;

      INT32 changeCSUniqueID( _dmsStorageUnit* su,
                              utilCSUniqueID csUniqueID ) ;

      INT32 changeUniqueID( const CHAR* csname,
                            utilCSUniqueID csUniqueID,
                            const BSONObj& clInfoObj,
                            BOOLEAN changeOtherCL,
                            const ossPoolVector<BSONObj>* pIdxInfoVec,
                            BOOLEAN changeIdx,
                            pmdEDUCB* cb,
                            SDB_DPSCB* dpsCB,
                            BOOLEAN isLoadCS = FALSE ) ;

      INT32 addCollectionSpace ( const CHAR *pName, UINT32 topSequence,
                                 _dmsStorageUnit *su, _pmdEDUCB *cb,
                                 SDB_DPSCB *dpsCB, BOOLEAN isCreate ) ;
      INT32 dropCollectionSpace ( const CHAR *pName, _pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB,
                                  dmsDropCSOptions *options = NULL ) ;
      INT32 dropEmptyCollectionSpace( const CHAR *pName, _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB,
                                      const CHAR *pExceptShortCLName = NULL ) ;
      INT32 unloadCollectonSpace( const CHAR *pName, _pmdEDUCB *cb ) ;

      INT32 renameCollectionSpace( const CHAR *pName,
                                   const CHAR *pNewName,
                                   _pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB ) ;
      INT32 renameCollectionSpaceP1( const CHAR *pName,
                                     const CHAR *pNewName,
                                     _pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;
      INT32 renameCollectionSpaceP1Cancel( const CHAR *pName,
                                           const CHAR *pNewName,
                                           _pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB ) ;
      INT32 renameCollectionSpaceP2( const CHAR *pName,
                                     const CHAR *pNewName,
                                     _pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;
      INT32 restoreCollectionSpace( const CHAR *pName ) ;
      INT32 moveCSToDeleting( const CHAR *pName ) ;
      INT32 returnCollectionSpaceP1( dmsReturnOptions &options,
                                     _pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;
      INT32 returnCollectionSpaceP1Cancel( dmsReturnOptions &options,
                                           _pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB ) ;
      INT32 returnCollectionSpaceP2( dmsReturnOptions &options,
                                     _pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;
      INT32 returnCollectionSpace( dmsReturnOptions &options,
                                   _pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB ) ;

      INT32 dumpInfo ( MON_CL_SIM_LIST &collectionList,
                       BOOLEAN sys = FALSE,
                       BOOLEAN dumpIdx = FALSE ) ;
      INT32 dumpInfo ( MON_CS_SIM_LIST &csList,
                       BOOLEAN sys = FALSE,
                       BOOLEAN dumpCL = FALSE,
                       BOOLEAN dumpIdx = FALSE ) ;

      INT32 dumpInfo ( MON_CL_LIST &collectionList,
                       BOOLEAN sys = FALSE ) ;
      INT32 dumpInfo ( MON_CS_LIST &csList,
                       BOOLEAN sys = FALSE,
                       BOOLEAN dumpIdx = FALSE ) ;
      INT32 dumpInfo ( MON_SU_LIST &storageUnitList,
                       BOOLEAN sys = FALSE ) ;

      INT32 dumpInfo( MON_CSNAME_VEC &vecCS,
                      BOOLEAN sys = FALSE,
                      dmsFilter *pFilter = NULL ) ;

      void dumpInfo ( INT64 &totalFileSize );

      INT32 dumpPageMapCSInfo( MON_CSNAME_VEC &vecCS ) ;

      UINT32 nullCSUniqueIDCnt() const ;

      dmsTempSUMgr *getTempSUMgr () ;

      dmsStatSUMgr *getStatSUMgr () ;

      dmsLocalSUMgr *getLocalSUMgr() ;

      dmsDataStatMgr *getStatMgr() ;

      void clearSUCaches ( UINT32 mask ) ;

      void clearSUCaches ( const MON_CS_SIM_LIST &monCSList, UINT32 mask ) ;

      void changeSUCaches ( UINT32 mask ) ;

      void changeSUCaches ( const MON_CS_SIM_LIST &monCSList, UINT32 mask ) ;

      INT32 dropCollectionSpaceP1 ( const CHAR *pName, _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

      INT32 dropCollectionSpaceP1Cancel ( const CHAR *pName, _pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB );

      INT32 dropCollectionSpaceP2 ( const CHAR *pName, _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB,
                                    dmsDropCSOptions *options = NULL ) ;

      BOOLEAN dispatchDictJob( dmsDictJob &job ) ;
      INT32 pushDictJob( const dmsDictJob &job ) ;

      BOOLEAN dispatchCheckItem( dmsCheckItem &item ) ;
      INT32 pushCheckItem( const dmsCheckItem &item ) ;

      void setIxmKeySorterCreator( dmsIxmKeySorterCreator* creator ) ;
      INT32 createIxmKeySorter( INT64 bufSize,
                                const _dmsIxmKeyComparer& comparer,
                                dmsIxmKeySorter** ppSorter ) ;
      void releaseIxmKeySorter( dmsIxmKeySorter* pSorter ) ;

      void setScannerCheckerCreator( IDmsScannerCheckerCreator *pCreator ) ;
      INT32 createScannerChecker( UINT32 suLID,
                                  UINT32 mbLID,
                                  const CHAR *csName,
                                  const CHAR *clShortName,
                                  const CHAR *optrDesc,
                                  _pmdEDUCB *cb,
                                  IDmsScannerChecker **ppChecker ) ;
      void releaseScannerChecker( IDmsScannerChecker *pChecker ) ;

      INT32 getMaxDMSLSN( DPS_LSN_OFFSET &maxLsn ) ;

   public:
      typedef std::vector<SDB_DMS_CSCB*>::iterator CSCB_ITERATOR;

      OSS_INLINE CSCB_ITERATOR begin()
      {
         return _cscbVec.begin();
      }

      OSS_INLINE CSCB_ITERATOR end()
      {
         return _cscbVec.end();
      }

      INT32 writable( _pmdEDUCB * cb ) ;
      void  writeDown( _pmdEDUCB * cb ) ;

      INT32 blockWrite( _pmdEDUCB *cb,
                        SDB_DB_STATUS byStatus = SDB_DB_NORMAL,
                        INT32 timeout = DMS_DFT_BLOCKWRITE_TIMEOUT ) ;
      void  unblockWrite( _pmdEDUCB *cb ) ;

      INT32 registerBackup( _pmdEDUCB *cb, BOOLEAN offline = TRUE ) ;
      void  backupDown( _pmdEDUCB *cb ) ;

      INT32 registerRebuild( _pmdEDUCB *cb ) ;
      void  rebuildDown( _pmdEDUCB *cb ) ;

      INT32 registerFullSync( _pmdEDUCB *cb ) ;
      void  fullSyncDown( _pmdEDUCB *cb ) ;

      OSS_INLINE UINT8 getCBState () const
      {
         return _dmsCBState ;
      }

      void  aquireCSMutex( const CHAR *pCSName ) ;
      void  releaseCSMutex( const CHAR *pCSName ) ;

      void clearAllCRUDCB () ;
      INT32 clearSUCRUDCB ( const CHAR * collectionSpace ) ;
      INT32 clearMBCRUDCB ( const CHAR * collection ) ;

      INT32 regHandler ( _IDmsEventHandler *pHandler ) ;
      void unregHandler ( _IDmsEventHandler *pHandler ) ;
   } ;
   typedef class _SDB_DMSCB SDB_DMSCB ;

   /*
      _dmsCSMutexScope define
   */
   class _dmsCSMutexScope
   {
      public:
         _dmsCSMutexScope( SDB_DMSCB *pDMSCB, const CHAR *pName ) ;
         ~_dmsCSMutexScope() ;

      private:
         SDB_DMSCB            *_pDMSCB ;
         const CHAR           *_pName ;
   } ;
   typedef _dmsCSMutexScope dmsCSMutexScope ;

   /*
      get global SDB_DMSCB
   */
   SDB_DMSCB* sdbGetDMSCB () ;
}

#endif //DMSCB_HPP_

