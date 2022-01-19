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

   Source File Name = dmsStorageUnit.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMSSTORAGEUNIT_HPP_
#define DMSSTORAGEUNIT_HPP_

#include "dmsStorageDataCommon.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageLob.hpp"
#include "monDMS.hpp"
#include "utilCache.hpp"
#include "dmsEventHandler.hpp"
#include "dmsExtDataHandler.hpp"
#include "dmsStatUnit.hpp"
#include "dmsCachedPlanUnit.hpp"
#include "ossMemPool.hpp"
#include "utilInsertResult.hpp"
#include "dmsEventHolder.hpp"
#include "dmsCacheHolder.hpp"

using namespace bson ;

namespace engine
{

   class _monCollection ;
   class _monStorageUnit ;
   class _monIndex ;
   class _ixmIndexCB ;
   class _dmsTempSUMgr ;
   class _SDB_DMSCB ;
   class _pmdEDUCB ;
   class _mthMatchTree ;
   class _mthMatchRuntime ;
   class _mthModifier ;

   /*
      _dmsStorageUnitStat define
   */
   struct _dmsStorageUnitStat
   {
      INT32          _clNum ;
      INT64          _totalCount ;
      INT32          _totalDataPages ;
      INT32          _totalIndexPages ;
      INT32          _totalLobPages ;
      INT64          _totalDataFreeSpace ;
      INT64          _totalIndexFreeSpace ;
   } ;
   typedef _dmsStorageUnitStat dmsStorageUnitStat ;

   #define DMS_SU_DATA           ( 0x0001 )
   #define DMS_SU_INDEX          ( 0x0002 )
   #define DMS_SU_LOB            ( 0x0004 )
   #define DMS_SU_ALL            ( 0xFFFF )

   /*
      _dmsStorageUnit define
   */
   class _dmsStorageUnit : public SDBObject
   {
      friend class _dmsTempSUMgr ;
      friend class _dmsRBSSUMgr ;
      friend class _SDB_DMSCB ;

      public:
         _dmsStorageUnit ( const CHAR *pSUName,
                           UINT32 csUniqueID,
                           UINT32 sequence,
                           utilCacheMgr *pMgr,
                           INT32 pageSize = DMS_PAGE_SIZE_DFT,
                           INT32 lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ,
                           DMS_STORAGE_TYPE type = DMS_STORAGE_NORMAL,
                           IDmsExtDataHandler *extDataHandler = NULL ) ;
         ~_dmsStorageUnit() ;

         INT32 open ( const CHAR *pDataPath,
                      const CHAR *pIndexPath,
                      const CHAR *pLobPath,
                      const CHAR *pLobMetaPath,
                      IDataSyncManager *pSyncMgr,
                      BOOLEAN createNew = TRUE ) ;
         void  close () ;
         INT32 remove () ;

         INT32 renameCS( const CHAR *pNewName ) ;

         INT32 setLobPageSize ( UINT32 lobPageSize ) ;

         dmsStorageDataCommon *data() { return _pDataSu ; }
         dmsStorageIndex   *index() { return _pIndexSu ; }
         dmsStorageLob     *lob() { return _pLobSu ; }
         utilCacheUnit     *cacheUnit() { return _pCacheUnit ; }

         INT32       getPageSize() const { return _storageInfo._pageSize ; }
         INT32       getLobPageSize() const { return _storageInfo._lobdPageSize ; }
         const CHAR* CSName() const { return _storageInfo._suName ; }
         UINT32      CSSequence() const { return _storageInfo._sequence ; }
         UINT32      LogicalCSID() const
         {
            return _pDataSu ? _pDataSu->logicalID() : DMS_INVALID_LOGICCSID ;
         }
         dmsStorageUnitID CSID() const
         {
            return _pDataSu ? _pDataSu->CSID() : DMS_INVALID_SUID ;
         }
         utilCSUniqueID CSUniqueID() const
         {
            return _storageInfo._csUniqueID ;
         }

         DMS_STORAGE_TYPE type() const { return _storageInfo._type ; }

         // for fast calculate page related values
         UINT32 getPageSizeLog2 () const
         {
            return _pDataSu ? _pDataSu->pageSizeSquareRoot() :
                              DMS_PAGE_SIZE_LOG2_DFT ;
         }

         INT64       totalSize ( UINT32 type = DMS_SU_ALL ) const ;
         INT64       totalDataPages( UINT32 type = DMS_SU_ALL ) const ;
         INT64       totalDataSize( UINT32 type = DMS_SU_ALL ) const ;
         INT64       totalFreePages( UINT32 type = DMS_SU_ALL ) const ;
         INT64       totalFreeSize( UINT32 type = DMS_SU_ALL ) const ;
         void        getStatInfo( dmsStorageUnitStat &statInfo ) ;

         INT32       sync( BOOLEAN sync,
                           IExecutor* cb ) ;

         void        enableSync( BOOLEAN enable ) ;
         void        restoreForCrash() ;

         void        setSyncConfig( UINT32 syncInterval,
                                    UINT32 syncRecordNum,
                                    UINT32 syncDirtyRatio ) ;
         void        setSyncDeep( BOOLEAN syncDeep ) ;
         void        setMVCCSupport( BOOLEAN mvccSupport ) ;

         UINT64      getCurrentDataLSN() const ;
         UINT64      getCurrentIdxLSN() const ;
         UINT64      getCurrentLobLSN() const ;
         void        getValidFlag( BOOLEAN &dataFlag,
                                   BOOLEAN &idxFlag,
                                   BOOLEAN &lobFlag ) const ;

      public:
         INT32    dumpInfo ( MON_CL_SIM_LIST &clList,
                             BOOLEAN sys = FALSE ) ;
         INT32    dumpInfo ( monCLSimple &collection,
                             dmsMBContext *context,
                             BOOLEAN dumpIdx = FALSE ) ;
         INT32    dumpInfo ( MON_CL_SIM_VEC &clList,
                             BOOLEAN sys = FALSE,
                             BOOLEAN dumpIdx = FALSE ) ;
         INT32    dumpInfo ( MON_CL_LIST &clList,
                             BOOLEAN sys = FALSE ) ;
         void     dumpInfo ( monStorageUnit &storageUnit ) ;
         INT32    dumpInfo ( monCSSimple &collectionSpace,
                             BOOLEAN sys = FALSE,
                             BOOLEAN dumpCL = FALSE,
                             BOOLEAN dumpIdx = FALSE ) ;
         INT32    dumpInfo ( monCollectionSpace &collectionSpace,
                             BOOLEAN sys = FALSE ) ;

         INT32    dumpCLInfo ( const CHAR *collectionName,
                               monCollection &info ) ;

         INT32    getSegExtents ( const CHAR *pName,
                                  vector< dmsExtentID > &segExtents,
                                  dmsMBContext *context = NULL ) ;

         INT32    getIndexes ( dmsMBContext *context,
                               MON_IDX_LIST &resultIndexes,
                               BOOLEAN excludeStandalone = FALSE ) ;

         INT32    getIndexes ( const CHAR *pName,
                               MON_IDX_LIST &resultIndexes,
                               BOOLEAN excludeStandalone = FALSE ) ;

         INT32    getIndex ( dmsMBContext *context,
                             const CHAR *pIndexName,
                             _monIndex &resultIndex ) ;

      protected :
         // Dump helper functions
         // NOTE: Should be called after mbContext is locked or
         //       metadataLatch is locked
         INT32    _dumpCLInfo ( monCollection &collection,
                                UINT16 mbID ) ;

         INT32    _dumpCLInfo ( monCLSimple &collection,
                                UINT16 mbID ) ;

         INT32    _getIndexes ( const dmsMB *mb,
                                MON_IDX_LIST &resultIndexes,
                                BOOLEAN excludeStandalone = FALSE ) ;

         INT32    _getIndex ( const dmsMB *mb,
                              const CHAR *pIndexName,
                              monIndex &resultIndex ) ;

      // only for LOAD
      public:
         OSS_INLINE void    mapExtent2DelList( dmsMB * mb, dmsExtent * extAddr,
                                               SINT32 extentID ) ;

         OSS_INLINE INT32   extentRemoveRecord( dmsMBContext *context,
                                                dmsExtRW &extRW,
                                                dmsRecordRW &recordRW,
                                                _pmdEDUCB *cb ) ;

         OSS_INLINE void    addExtentRecordCount( dmsMB *mb, UINT32 count ) ;

      // for dmsCB
      protected:
         OSS_INLINE void  _setLogicalCSID( UINT32 logicalID ) ;

         OSS_INLINE void  _setCSID( dmsStorageUnitID CSID ) ;

         INT32        _resetCollection( dmsMBContext *context ) ;
      // for dmsCB
      private:
         dmsStorageInfo* _getStorageInfo() { return &_storageInfo ; }

      public:
         // Position is used to specify the insert position of the record.
         // Currently it's used in capped collection, holding the logical id
         // of the record.
         INT32    insertRecord ( const CHAR *pName,
                                 const BSONObj &record,
                                 _pmdEDUCB *cb,
                                 SDB_DPSCB *dpscb,
                                 BOOLEAN mustOID = TRUE,
                                 BOOLEAN canUnLock = TRUE,
                                 dmsMBContext *context = NULL,
                                 INT64 position = -1,
                                 utilInsertResult *insertResult = NULL ) ;

         INT32    updateRecords ( const CHAR *pName,
                                  _pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb,
                                  _mthMatchRuntime *matchRuntime,
                                  _mthModifier &modifier,
                                  SINT64 maxUpdate = -1,
                                  dmsMBContext *context = NULL,
                                  utilUpdateResult *pResult = NULL ) ;

         INT32    deleteRecords ( const CHAR *pName,
                                  _pmdEDUCB * cb,
                                  SDB_DPSCB *dpscb,
                                  _mthMatchRuntime *matchRuntime,
                                  SINT64 maxDelete = -1,
                                  dmsMBContext *context = NULL,
                                  utilDeleteResult *pResult = NULL ) ;

         INT32    rebuildIndexes ( const CHAR *pName,
                                   _pmdEDUCB * cb,
                                   dmsMBContext *context = NULL ) ;

         INT32    createIndex ( const CHAR *pName, const BSONObj &index,
                                _pmdEDUCB * cb, SDB_DPSCB *dpscb,
                                BOOLEAN isSys = FALSE,
                                dmsMBContext *context = NULL,
                                INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                utilWriteResult *pResult = NULL,
                                dmsIdxTaskStatus *pIdxStatus = NULL,
                                BOOLEAN forceTransCallback = FALSE,
                                BOOLEAN addUIDIfNotExist = TRUE ) ;

         INT32    createIndex ( utilCLUniqueID clUniqID,
                                const BSONObj &index,
                                _pmdEDUCB * cb, SDB_DPSCB *dpscb,
                                BOOLEAN isSys = FALSE,
                                dmsMBContext *context = NULL,
                                INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                utilWriteResult *pResult = NULL,
                                dmsIdxTaskStatus *pIdxStatus = NULL,
                                BOOLEAN forceTransCallback = FALSE,
                                BOOLEAN addUIDIfNotExist = TRUE ) ;

         INT32    dropIndex( const CHAR *pName, const CHAR *indexName,
                             _pmdEDUCB * cb, SDB_DPSCB *dpscb,
                             BOOLEAN isSys = FALSE,
                             dmsMBContext *context = NULL,
                             dmsIdxTaskStatus *pIdxStatus = NULL,
                             BOOLEAN onlyStandalone = FALSE ) ;

         INT32    dropIndex( const CHAR *pName, OID &indexOID,
                             _pmdEDUCB * cb, SDB_DPSCB *dpscb,
                             BOOLEAN isSys = FALSE,
                             dmsMBContext *context = NULL,
                             dmsIdxTaskStatus *pIdxStatus = NULL,
                             BOOLEAN onlyStandalone = FALSE ) ;

         INT32    dropIndex( utilCLUniqueID clUniqID, const CHAR *indexName,
                             _pmdEDUCB * cb, SDB_DPSCB *dpscb,
                             BOOLEAN isSys = FALSE,
                             dmsMBContext *context = NULL,
                             dmsIdxTaskStatus *pIdxStatus = NULL,
                             BOOLEAN onlyStandalone = FALSE ) ;

         INT32    dropIndex( utilCLUniqueID clUniqID, OID &indexOID,
                             _pmdEDUCB * cb, SDB_DPSCB *dpscb,
                             BOOLEAN isSys = FALSE,
                             dmsMBContext *context = NULL,
                             dmsIdxTaskStatus *pIdxStatus = NULL,
                             BOOLEAN onlyStandalone = FALSE ) ;

         INT32    countCollection ( const CHAR *pName,
                                    INT64 &recordNum,
                                    _pmdEDUCB *cb,
                                    dmsMBContext *context = NULL ) ;

         INT32    getCollectionFlag ( const CHAR *pName, UINT16 &flag,
                                      dmsMBContext *context = NULL ) ;

         INT32    changeCollectionFlag ( const CHAR *pName, UINT16 flag,
                                         dmsMBContext *context = NULL ) ;

         INT32    getCollectionAttributes ( const CHAR *pName,
                                            UINT32 &attributes,
                                            dmsMBContext *context = NULL ) ;

         INT32    updateCollectionAttributes ( const CHAR *pName,
                                               UINT32 newAttributes,
                                               dmsMBContext *context = NULL ) ;

         INT32    getCollectionCompType ( const CHAR *pName,
                                          UTIL_COMPRESSOR_TYPE &compType,
                                          dmsMBContext *context = NULL ) ;

         INT32    setCollectionCompType ( const CHAR * pName,
                                          UTIL_COMPRESSOR_TYPE compType,
                                          dmsMBContext * context = NULL ) ;

         INT32    setCollectionAttribute ( const CHAR * pName,
                                           UINT32 attributeMask,
                                           BOOLEAN attributeValue,
                                           dmsMBContext * context = NULL ) ;

         INT32    setCollectionStrictDataMode ( const CHAR * pName,
                                                BOOLEAN strictDataMode,
                                                dmsMBContext * context = NULL ) ;

         INT32    setCollectionNoIDIndex ( const CHAR * pName,
                                           BOOLEAN noIDIndex,
                                           dmsMBContext * context = NULL ) ;

         INT32    canSetCollectionCompressor ( dmsMBContext * context ) ;
         INT32    setCollectionCompressor ( const CHAR * pName,
                                            UTIL_COMPRESSOR_TYPE compressType,
                                            dmsMBContext * context = NULL ) ;

         INT32    getCollectionExtOptions( const CHAR *pName,
                                           BSONObj &extOptions,
                                           dmsMBContext *context = NULL ) ;

         INT32    setCollectionExtOptions ( const CHAR * pName,
                                            const BSONObj & extOptions,
                                            dmsMBContext * context = NULL ) ;

         //loadExtentA is not init extent records
         INT32    loadExtentA ( dmsMBContext *mbContext, const CHAR *pBuffer,
                                UINT16 numPages, const BOOLEAN toLoad = FALSE,
                                SINT32 *allocatedExtent = NULL ) ;

         //loadExtent will init extent records
         INT32    loadExtent ( dmsMBContext *mbContext, const CHAR *pBuffer,
                               UINT16 numPages ) ;

      public :
         _IDmsEventHolder * getEventHolder () ;

         void regEventHandler ( _IDmsEventHandler *pHandler ) ;
         void unregEventHandler ( _IDmsEventHandler *pHandler ) ;
         void unregEventHandlers () ;

         dmsSUCache *getSUCache ( UINT32 type ) ;

         dmsStatCache *getStatCache () ;
         dmsCachedPlanMgr *getCachedPlanMgr () ;

      public :
         // monitor CRUD helper functions
         void clearMBCRUDCB () ;

      private:
         INT32 _createStorageObjs() ;
         INT32 _getTypeFromFile( const CHAR *dataPath,
                                 DMS_STORAGE_TYPE &type ) ;

      private:
         INT32 createCSInDataEngine();
         INT32 getRecordCountInEngine(pmdEDUCB *cb,
                                      dmsMBContext *context,
                                      UINT64 &count);
         INT32 removeCSInEngine(pmdEDUCB *cb);

      private :
         dmsStorageDataCommon                *_pDataSu ;
         dmsStorageIndex                     *_pIndexSu ;
         dmsStorageInfo                      _storageInfo ;
         dmsStorageLob                       *_pLobSu ;

         utilCacheMgr                        *_pMgr ;
         utilCacheUnit                       *_pCacheUnit ;
         dmsEventHolder                       _eventHolder ;
         dmsCacheHolder                       _cacheHolder ;
   } ;

   typedef class _dmsStorageUnit dmsStorageUnit;

   OSS_INLINE INT32 _dmsStorageUnit::extentRemoveRecord( dmsMBContext *context,
                                                         dmsExtRW &extRW,
                                                         dmsRecordRW &recordRW,
                                                         _pmdEDUCB *cb )
   {
      return _pDataSu->_extentRemoveRecord( context, extRW, recordRW, cb ) ;
   }
   OSS_INLINE void _dmsStorageUnit::addExtentRecordCount( dmsMB * mb, UINT32 count )
   {
      _pDataSu->_mbStatInfo[ mb->_blockID ]._totalRecords += count ;
      _pDataSu->_mbStatInfo[ mb->_blockID ]._rcTotalRecords.add( count ) ;
   }
   OSS_INLINE void _dmsStorageUnit::_setLogicalCSID( UINT32 logicalID )
   {
      if ( _pDataSu )
      {
         _pDataSu->_logicalCSID = logicalID ;
      }
   }
   OSS_INLINE void _dmsStorageUnit::_setCSID( dmsStorageUnitID CSID )
   {
      if ( _pDataSu )
      {
         _pDataSu->_CSID = CSID ;
      }
   }
}

#endif //DMSSTORAGEUNIT_HPP_

