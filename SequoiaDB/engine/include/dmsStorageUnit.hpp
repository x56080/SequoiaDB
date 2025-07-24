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

#include "dmsStorageData.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageLob.hpp"
#include "rtnAPM.hpp"
#include "monDMS.hpp"
#include "utilCache.hpp"

using namespace bson ;

namespace engine
{

   class _monCollection ;
   class _monStorageUnit ;
   class _monIndex ;
   class _ixmIndexCB ;
   class _dmsTempCB ;
   class _SDB_DMSCB ;
   class _pmdEDUCB ;
   class _mthMatcher ;
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
      friend class _dmsTempCB ;
      friend class _SDB_DMSCB ;

      public:
         _dmsStorageUnit ( const CHAR *pSUName,
                           UINT32 sequence,
                           utilCacheMgr *pMgr,
                           INT32 pageSize = DMS_PAGE_SIZE_DFT,
                           INT32 lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ) ;
         ~_dmsStorageUnit() ;

         INT32 open ( const CHAR *pDataPath, const CHAR *pIndexPath,
                      const CHAR *pLobPath,
                      BOOLEAN createNew = TRUE,
                      BOOLEAN delWhenExist = FALSE ) ;
         void  close () ;
         INT32 remove () ;

         dmsStorageData    *data() { return _pDataSu ; }
         dmsStorageIndex   *index() { return _pIndexSu ; }
         dmsStorageLob     *lob() { return _pLobSu ; }
         rtnAccessPlanManager *getAPM () { return &_apm ; }

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

         INT64       totalSize ( UINT32 type = DMS_SU_ALL ) const ;
         INT64       totalDataPages( UINT32 type = DMS_SU_ALL ) const ;
         INT64       totalDataSize( UINT32 type = DMS_SU_ALL ) const ;
         INT64       totalFreePages( UINT32 type = DMS_SU_ALL ) const ;
         INT64       totalFreeSize( UINT32 type = DMS_SU_ALL ) const ;
         void        getStatInfo( dmsStorageUnitStat &statInfo ) ;
         INT32       tryToFlush( BOOLEAN ignoreTick, BOOLEAN &failed ) ;
         UINT64      getCurrentDataLSN() const ;
         UINT64      getCurrentLobLSN() const ;
         std::string getValidFlagDesc() const ;
         UINT32      getValidFlag() const ;
         void        resetLastLSN( UINT64 offset ) ;
         UINT64      getLastTick() const ;

      public:
         void     dumpInfo ( set<monCLSimple> &collectionList,
                             BOOLEAN sys = FALSE ) ;
         void     dumpInfo ( vector<monCLSimple> &collectionList,
                             BOOLEAN sys = FALSE ) ;
         void     dumpInfo ( set<_monCollection> &collectionList,
                             BOOLEAN sys = FALSE ) ;
         void     dumpInfo ( set<_monStorageUnit> &storageUnitList,
                             BOOLEAN sys = FALSE ) ;

         INT32    getSegExtents ( const CHAR *pName,
                                  vector< dmsExtentID > &segExtents,
                                  dmsMBContext *context = NULL ) ;

         INT32    getIndexes ( const CHAR *pName,
                               vector<_monIndex> &resultIndexes,
                               dmsMBContext *context = NULL ) ;

         INT32    getIndex( const CHAR *pName,
                            const CHAR *pIndexName,
                            _monIndex &resultIndex,
                            dmsMBContext *context = NULL ) ;

      // only for LOAD
      public:
         OSS_INLINE void    mapExtent2DelList( dmsMB * mb, dmsExtent * extAddr,
                                           SINT32 extentID ) ;

         OSS_INLINE INT32   extentRemoveRecord( dmsMB *mb,
                                            const dmsRecordID &recordID,
                                            INT32 recordSize,
                                            _pmdEDUCB *cb ) ;

         OSS_INLINE void    addExtentRecordCount( dmsMB *mb, UINT32 count ) ;

      // for dmsCB
      protected:
         OSS_INLINE void  _setLogicalCSID( UINT32 logicalID ) ;

         OSS_INLINE void  _setCSID( dmsStorageUnitID CSID ) ;

         INT32        _resetCollection( dmsMBContext *context ) ;

      public:

         INT32    insertRecord ( const CHAR *pName,
                                 BSONObj &record,
                                 _pmdEDUCB *cb,
                                 SDB_DPSCB *dpscb,
                                 BOOLEAN mustOID = TRUE,
                                 BOOLEAN canUnLock = TRUE,
                                 dmsMBContext *context = NULL ) ;

         INT32    updateRecords ( const CHAR *pName,
                                  _pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb,
                                  _mthMatcher *matcher,
                                  _mthModifier &modifier,
                                  SINT64 &numRecords,
                                  SINT64 maxUpdate = -1,
                                  dmsMBContext *context = NULL ) ;

         INT32    deleteRecords ( const CHAR *pName,
                                  _pmdEDUCB * cb,
                                  SDB_DPSCB *dpscb,
                                  _mthMatcher *matcher,
                                  SINT64 &numRecords,
                                  SINT64 maxDelete = -1,
                                  dmsMBContext *context = NULL ) ;

         INT32    rebuildIndexes ( const CHAR *pName,
                                   _pmdEDUCB * cb,
                                   dmsMBContext *context = NULL ) ;

         INT32    createIndex ( const CHAR *pName, const BSONObj &index,
                                _pmdEDUCB * cb, SDB_DPSCB *dpscb,
                                BOOLEAN isSys = FALSE,
                                dmsMBContext *context = NULL,
                                INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ) ;

         INT32    dropIndex( const CHAR *pName, const CHAR *indexName,
                             _pmdEDUCB * cb, SDB_DPSCB *dpscb,
                             BOOLEAN isSys = FALSE,
                             dmsMBContext *context = NULL ) ;

         INT32    dropIndex( const CHAR *pName, OID &indexOID,
                             _pmdEDUCB * cb, SDB_DPSCB *dpscb,
                             BOOLEAN isSys = FALSE,
                             dmsMBContext *context = NULL ) ;

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
         //loadExtentA is not init extent records
         INT32    loadExtentA ( dmsMBContext *mbContext, const CHAR *pBuffer,
                                UINT16 numPages, const BOOLEAN toLoad = FALSE,
                                SINT32 *allocatedExtent = NULL,
                                dmsExtent **tExtAddr = NULL ) ;

         //loadExtent will init extent records
         INT32    loadExtent ( dmsMBContext *mbContext, const CHAR *pBuffer,
                               UINT16 numPages ) ;

      private :
         rtnAccessPlanManager                _apm ;

         dmsStorageData                      *_pDataSu ;
         dmsStorageIndex                     *_pIndexSu ;
         dmsStorageInfo                      _storageInfo ;
         dmsStorageLob                       *_pLobSu ;

         utilCacheMgr                        *_pMgr ;
         utilCacheUnit                       *_pCacheUnit ;

   } ;
   typedef _dmsStorageUnit dmsStorageUnit ;

   /*
      _dmsStorageUnit OSS_INLINE functions
   */
   OSS_INLINE void _dmsStorageUnit::mapExtent2DelList( dmsMB * mb,
                                                   dmsExtent *extAddr,
                                                   SINT32 extentID )
   {
      return _pDataSu->_mapExtent2DelList( mb, extAddr, extentID ) ;
   }
   OSS_INLINE INT32 _dmsStorageUnit::extentRemoveRecord(dmsMB * mb,
                                                    const dmsRecordID &recordID,
                                                    INT32 recordSize,
                                                    _pmdEDUCB *cb )
   {
      return _pDataSu->_extentRemoveRecord( mb, recordID, recordSize, cb ) ;
   }
   OSS_INLINE void _dmsStorageUnit::addExtentRecordCount( dmsMB * mb, UINT32 count )
   {
      _pDataSu->_mbStatInfo[ mb->_blockID ]._totalRecords += count ;
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

