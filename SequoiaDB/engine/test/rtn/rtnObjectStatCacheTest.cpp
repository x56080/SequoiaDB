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

   
*******************************************************************************/
#include "rtnObjectStatCache.hpp"
#include "dmsStatSUMgr.hpp"
#include "dmsObjectMetaInfo.hpp"
#include "interface/IDataCollection.h"
#include "interface/IDataManagementService.h"
#include "msgDef.h"
#include "../testDef.h"
#include "sdbInterface.hpp"
#include "utilStringView.hpp"
#include "utilUniqueID.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <unordered_map>
namespace engine
{
   constexpr const CHAR SAMPLE_CS_NAME[] = "cs";
   constexpr utilCSUniqueID SAMPLE_CS_UID = 1;
   constexpr const CHAR SAMPLE_CL_NAME_1[] = "cs.cl1";
   const std::shared_ptr< const ossPoolString > SAMPLE_CL_NAME_1_PTR =
      makeSharedPtrFromPool< ossPoolString >( SAMPLE_CL_NAME_1 );
   constexpr const CHAR SAMPLE_CL_NAME_2[] = "cs.cl2";
   const std::shared_ptr< const ossPoolString > SAMPLE_CL_NAME_2_PTR =
      makeSharedPtrFromPool< ossPoolString >( SAMPLE_CL_NAME_2 );
   const utilCLUniqueID SAMPLE_CL_UID_1 = utilBuildCLUniqueID( SAMPLE_CS_UID, 1 );
   const utilCLUniqueID SAMPLE_CL_UID_2 = utilBuildCLUniqueID( SAMPLE_CS_UID, 2 );
   constexpr const CHAR SAMPLE_INDEX_NAME_CL1_1[] = "index1_1";
   const std::shared_ptr< const ossPoolString > SAMPLE_INDEX_NAME_CL1_1_PTR =
      makeSharedPtrFromPool< ossPoolString >( SAMPLE_INDEX_NAME_CL1_1 );
   constexpr utilIdxInnerID SAMPLE_INDEX_INNER_ID_CL1_1 = 11;
   constexpr const CHAR SAMPLE_INDEX_NAME_CL1_2[] = "index1_2";
   const std::shared_ptr< const ossPoolString > SAMPLE_INDEX_NAME_CL1_2_PTR =
      makeSharedPtrFromPool< ossPoolString >( SAMPLE_INDEX_NAME_CL1_2 );
   constexpr utilIdxInnerID SAMPLE_INDEX_INNER_ID_CL1_2 = 12;
   constexpr const CHAR SAMPLE_INDEX_NAME_CL2_1[] = "index2_1";
   const std::shared_ptr< const ossPoolString > SAMPLE_INDEX_NAME_CL2_1_PTR =
      makeSharedPtrFromPool< ossPoolString >( SAMPLE_INDEX_NAME_CL2_1 );
   constexpr utilIdxInnerID SAMPLE_INDEX_INNER_ID_CL2_1 = 21;
   constexpr const CHAR SAMPLE_INDEX_NAME_CL2_2[] = "index2_2";
   const std::shared_ptr< const ossPoolString > SAMPLE_INDEX_NAME_CL2_2_PTR =
      makeSharedPtrFromPool< ossPoolString >( SAMPLE_INDEX_NAME_CL2_2 );
   constexpr utilIdxInnerID SAMPLE_INDEX_INNER_ID_CL2_2 = 22;

   RTN_INDEX_STAT_PTR buildRtnIndexStatInfo(
      const std::shared_ptr< const ossPoolString > &clFullName,
      const std::shared_ptr< const ossPoolString > &indexName,
      UINT32 indexPages,
      UINT32 indexLevels,
      BOOLEAN isUnique,
      const BSONObj &keyPattern )
   {
      RTN_INDEX_STAT_PTR stat = makeSharedPtrFromPool< rtnIndexStatInfo >( clFullName, indexName );
      stat->setIndexPages( indexPages );
      stat->setIndexLevels( indexLevels );
      stat->setUnique( isUnique );
      stat->setKeyPattern( keyPattern );
      constexpr UINT32 SAMPLE_SIZE = 200;
      for ( UINT32 i = 0; i < SAMPLE_SIZE; ++i )
      {
         stat->pushMCVSet( BSON( "a" << i ), 1.0 / SAMPLE_SIZE );
      }
      stat->postInit();
      return stat;
   }

   class collectionTest : public IDataCollection
   {
   public:
      collectionTest( const ossPoolString clFullName,
                      utilCLUniqueID cluid,
                      UINT32 attributes,
                      UINT32 pageSizeLog2,
                      UINT32 totalDataPages,
                      std::vector< DMS_INDEX_META_PTR > &&vecIndexMeta,
                      RTN_CL_STAT_PTR &&clStatPtr,
                      std::vector< RTN_INDEX_STAT_PTR > &&vecIndexStat )
      : _clFullName( clFullName )
      , _clUID( cluid )
      , _attributes( attributes )
      , _pageSizeLog2( pageSizeLog2 )
      , _totalDataPages( totalDataPages )
      , _vecIndexMeta( std::move( vecIndexMeta ) )
      , _clStatPtr( std::move( clStatPtr ) )
      , _vecIndexStat( std::move( vecIndexStat ) )
      {
      }

   public:
      virtual BOOLEAN isClosed() const override
      {
         return _closed;
      }
      virtual void close() override
      {
         _closed = TRUE;
      }

   public:
      virtual INT32 createIndex( IExecutor *executor,
                                 const dmsBuildIndexOptions &o,
                                 const bson::BSONObj &indexDef ) override
      {
         return SDB_OK;
      }

      virtual INT32 getMetaData( IExecutor *executor, CONST_CL_META_INFO_PTR &meta ) override
      {
         DMS_CL_META_PTR clMetaInfo = makeSharedPtrFromPool< dmsCollectionMetaInfo >(
            makeSharedPtrFromPool< ossPoolString >( _clFullName ), _clUID, _attributes,
            _pageSizeLog2, _totalDataPages, _globTransAvailTime );

         for ( auto it = _vecIndexMeta.begin(); it != _vecIndexMeta.end(); ++it )
         {
            clMetaInfo->pushIndexMetaInfo( *it );
         }
         meta = clMetaInfo;
         return SDB_OK;
      }

      virtual DMS_STORAGE_TYPE getCSStorageType() override
      {
         return DMS_STORAGE_NORMAL;
      }

      virtual INT32 listIndex( IExecutor *executor,
                               ossPoolVector< bson::BSONObj > &indexes ) override
      {

         return SDB_OK;
      }

      virtual INT32 removeIndex( IExecutor *executor,
                                 const CHAR *indexName ) override
      {
         return SDB_OK;
      }

      virtual INT32 removeIndex( IExecutor *executor,
                                 const CHAR *indexName,
                                 const dmsRemoveIndexOptions & ) override
      {
         return SDB_OK;
      }

      virtual INT32 removeIndex( IExecutor *executor,
                                 const OID &indexOID,
                                 const dmsRemoveIndexOptions &o ) override
      {
         return SDB_OK;
      }

   public:
      virtual INT32 truncate( IExecutor *executor, const dmsTruncateCLOptions &o ) override
      {
         return SDB_OK;
      }

   public:
      virtual INT32 insertRecord( IExecutor *executor,
                                  const bson::BSONObj &record,
                                  const dmsInsertRecordOptions &o,
                                  utilInsertResult *result ) override
      {
         return SDB_OK;
      }
      virtual INT32 insertBatch( IExecutor *executor,
                                 const ossPoolVector< bson::BSONObj > &batch,
                                 const dmsInsertRecordOptions &o,
                                 utilInsertResult *result ) override
      {
         return SDB_OK;
      }

      virtual INT32 updateRecord( IExecutor *executor,
                                  const dmsRecordID &rid,
                                  IRecordUpdater *updater,
                                  const dmsUpdateRecordOptions &o,
                                  utilUpdateResult *result ) override
      {
         return SDB_OK;
      }

      virtual INT32 deleteRecord( IExecutor *executor,
                                  const dmsRecordID &rid,
                                  const dmsDeleteRecordOptions &o,
                                  utilDeleteResult *result ) override
      {
         return SDB_OK;
      }

   public:
      virtual INT32 scan( IExecutor *executor,
                          const dmsScanOptions &o,
                          DATA_CURSOR_PTR &cursor ) override
      {
         return SDB_OK;
      }

      virtual INT32 scanIndex( IExecutor *executor,
                               const CHAR *indexName,
                               const rtnPredicateList &predicate,
                               const dmsIndexScanOptions &o,
                               DATA_CURSOR_PTR &cursor ) override
      {
         return SDB_OK;
      }

      virtual INT32 getRecordCount( IExecutor *executor, UINT64 &count ) override
      {
         return SDB_OK;
      }

   public: /// lob
      virtual INT32 insertLobChunk( IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    UINT32 offset,
                                    UINT32 size,
                                    const CHAR *data ) override
      {
         return SDB_OK;
      }

      virtual INT32 readLobChunk( IExecutor *executor,
                                  const bson::OID &oid,
                                  UINT32 chunkId,
                                  UINT32 offset,
                                  UINT32 size,
                                  CHAR *data,
                                  UINT32 &readSize ) override
      {
         return SDB_OK;
      }

      virtual INT32 removeLobChunk( IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId ) override
      {
         return SDB_OK;
      }

      virtual INT32 updateLobChunk( IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    UINT32 offset,
                                    UINT32 size,
                                    const CHAR *data,
                                    BOOLEAN createIfNotExists ) override
      {
         return SDB_OK;
      }

      virtual INT32 truncateLobChunk( IExecutor *executor,
                                      const bson::OID &oid,
                                      UINT32 chunkId,
                                      UINT32 size,
                                      UINT32 &tsize ) override
      {
         return SDB_OK;
      }

      virtual INT32 listLobChunks( IExecutor *executor,
                                   const dmsListLobChunkOptions &o,
                                   DATA_CURSOR_PTR &cursor ) override
      {
         return SDB_OK;
      }

      virtual INT32 testLobChunk( IExecutor *executor,
                                  const bson::OID &oid,
                                  UINT32 chunkId,
                                  dmsLobChunkProfile *profile ) override
      {
         return SDB_OK;
      }

   public:
      INT32 calledTimes = 0;

   public:
      ossPoolString _clFullName;
      utilCLUniqueID _clUID = UTIL_UNIQUEID_NULL;
      UINT32 _attributes = 0;
      UINT32 _pageSizeLog2 = 16;
      UINT32 _totalDataPages = RTN_STAT_DEF_TOTAL_PAGES;
      UINT64 _globTransAvailTime = DPS_MAX_TRANS_TIME;
      std::vector< DMS_INDEX_META_PTR > _vecIndexMeta;
      RTN_CL_STAT_PTR _clStatPtr = nullptr;
      std::vector< RTN_INDEX_STAT_PTR > _vecIndexStat;
      BOOLEAN _closed = FALSE;
   };

   class dmsTest : public IDataManagementService
   {
   public:
      virtual INT32 createCS( IExecutor *executor,
                              const CHAR *name,
                              utilCSUniqueID uniqueId,
                              const dmsCreateCSOptions &o,
                              const bson::BSONObj &adjunct ) override
      {
         return SDB_OK;
      }

      virtual INT32 dropCS( IExecutor *executor,
                            const CHAR *name,
                            const dmsRemoveCSOptions &options ) override
      {
         return SDB_OK;
      }

      virtual INT32 renameCS( IExecutor *executor,
                              const CHAR *oldName,
                              const CHAR *newName,
                              BOOLEAN blockWrite ) override
      {
         return SDB_OK;
      }

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
                              const bson::BSONObj &adjunct ) override
      {
         return SDB_OK;
      }

      virtual INT32 dropCL( IExecutor *executor,
                            const CHAR *clFullName,
                            const dmsRemoveCLOptions &o ) override
      {
         return SDB_OK;
      }

      virtual INT32 nameToSuDescriptor( const CHAR *pName, DMS_SU_DESCRIPTOR &desc ) override
      {
         return SDB_OK;
      }

      virtual UINT32 getNullCSUniqueIDCnt() const override
      {
         return 0;
      }
   public:
      std::shared_ptr< collectionTest > getCL( const CHAR *fullName )
      {
         return colls[ fullName ];
      }

   public:
      std::unordered_map< std::string, std::shared_ptr< collectionTest > > colls{
         { SAMPLE_CL_NAME_1,
           makeSharedPtrFromPool< collectionTest >(
              SAMPLE_CL_NAME_1,
              SAMPLE_CL_UID_1,
              1,
              16,
              100,
              std::vector< DMS_INDEX_META_PTR >{
                 { makeSharedPtrFromPool< dmsIndexMetaInfo >( SAMPLE_CL_NAME_1_PTR,
                                                              SAMPLE_INDEX_NAME_CL1_1_PTR,
                                                              SAMPLE_CL_UID_1,
                                                              SAMPLE_INDEX_INNER_ID_CL1_1,
                                                              OID::gen(),
                                                              BSON( "a" << 1 << "b" << 1 ),
                                                              1,
                                                              1,
                                                              1,
                                                              0,
                                                              0,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE ) },
                 { makeSharedPtrFromPool< dmsIndexMetaInfo >( SAMPLE_CL_NAME_1_PTR,
                                                              SAMPLE_INDEX_NAME_CL1_2_PTR,
                                                              SAMPLE_CL_UID_1,
                                                              SAMPLE_INDEX_INNER_ID_CL1_2,
                                                              OID::gen(),
                                                              BSON( "a" << -1 << "b" << -1 ),
                                                              2,
                                                              2,
                                                              2,
                                                              0,
                                                              0,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE ) } },
              makeSharedPtrFromPool< rtnCollectionStatInfo >( SAMPLE_CL_NAME_1_PTR,
                                                              1,
                                                              200,
                                                              600000,
                                                              1284,
                                                              65929411,
                                                              10 ),
              std::vector< RTN_INDEX_STAT_PTR >{
                 buildRtnIndexStatInfo( SAMPLE_CL_NAME_1_PTR,
                                        SAMPLE_INDEX_NAME_CL1_1_PTR,
                                        256,
                                        2,
                                        FALSE,
                                        BSON( "a" << 1 << "b" << 1 ) ),
                 buildRtnIndexStatInfo( SAMPLE_CL_NAME_1_PTR,
                                        SAMPLE_INDEX_NAME_CL1_2_PTR,
                                        256,
                                        2,
                                        FALSE,
                                        BSON( "a" << -1 << "b" << -1 ) ) } ) },

         { SAMPLE_CL_NAME_2,
           makeSharedPtrFromPool< collectionTest >(
              SAMPLE_CL_NAME_2,
              SAMPLE_CL_UID_2,
              1,
              16,
              100,
              std::vector< DMS_INDEX_META_PTR >{
                 { makeSharedPtrFromPool< dmsIndexMetaInfo >( SAMPLE_CL_NAME_2_PTR,
                                                              SAMPLE_INDEX_NAME_CL2_1_PTR,
                                                              SAMPLE_CL_UID_2,
                                                              SAMPLE_INDEX_INNER_ID_CL2_1,
                                                              OID::gen(),
                                                              BSON( "a" << 1 << "b" << 1 ),
                                                              3,
                                                              3,
                                                              3,
                                                              0,
                                                              0,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE ) },
                 { makeSharedPtrFromPool< dmsIndexMetaInfo >( SAMPLE_CL_NAME_2_PTR,
                                                              SAMPLE_INDEX_NAME_CL2_2_PTR,
                                                              SAMPLE_CL_UID_2,
                                                              SAMPLE_INDEX_INNER_ID_CL2_2,
                                                              OID::gen(),
                                                              BSON( "a" << -1 << "b" << -1 ),
                                                              4,
                                                              4,
                                                              4,
                                                              0,
                                                              0,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE,
                                                              FALSE ) } },
              makeSharedPtrFromPool< rtnCollectionStatInfo >( SAMPLE_CL_NAME_2_PTR,
                                                              1,
                                                              200,
                                                              600000,
                                                              1284,
                                                              65929411,
                                                              10 ),
              std::vector< RTN_INDEX_STAT_PTR >{
                 buildRtnIndexStatInfo( SAMPLE_CL_NAME_2_PTR,
                                        SAMPLE_INDEX_NAME_CL2_1_PTR,
                                        256,
                                        2,
                                        FALSE,
                                        BSON( "a" << 1 << "b" << 1 ) ),
                 buildRtnIndexStatInfo( SAMPLE_CL_NAME_2_PTR,
                                        SAMPLE_INDEX_NAME_CL2_2_PTR,
                                        256,
                                        2,
                                        FALSE,
                                        BSON( "a" << -1 << "b" << -1 ) ) } ) } };
   };

   class rtn_object_stat_cache_test : public testing::Test
   {
   public:
      static void SetUpTestCase()
      {
         sdbEnablePD( "/opt/diaglog/sdb.log", 1, 1000 );
         setPDLevel( PDDEBUG );
      }

      static void TearDownTestCase() {}

      virtual void SetUp() override
      {
         contexts.clear();
         getMorePos.clear();
         statCache.init(
            newRtnObjectStatAgentImpl( &dms, queryFunc, getMoreFunc, killContextFunc ) );
      }

      virtual void TearDown() override
      {
         statCache.fini();
      }

      BOOLEAN checkCollectionStat( CONST_CL_STAT_INFO_PTR actStat, const collectionTest &expCL )
      {
         if ( !actStat )
         {
            return FALSE;
         }
         CONST_RTN_CL_STAT_PTR expCLStat = expCL._clStatPtr;
         if ( utilStringView( actStat->getCLFullName() ) ==
                 utilStringView( expCLStat->getCLFullName() ) &&
              actStat->getCreateTime() == expCLStat->getCreateTime() &&
              actStat->getSampleRecords() == expCLStat->getSampleRecords() &&
              actStat->getTotalRecords() == expCLStat->getTotalRecords() &&
              actStat->getTotalDataPages() == expCLStat->getTotalDataPages() &&
              actStat->getTotalDataSize() == expCLStat->getTotalDataSize() &&
              actStat->getAvgNumFields() == expCLStat->getAvgNumFields() &&
              actStat->getIndexNum() == expCL._vecIndexStat.size() )
         {
            return TRUE;
         }
         return FALSE;
      }

   public:
      dmsTest dms;
      std::unordered_map< INT64, std::vector< BSONObj > > contexts;
      std::unordered_map< INT64, UINT32 > getMorePos;
      INT64 curContextID = -1;
      rtnObjectStatCache statCache;
      std::function< INT32( const CHAR *,
                            const BSONObj &,
                            const BSONObj &,
                            const BSONObj &,
                            const BSONObj &,
                            SINT32,
                            SINT64,
                            SINT64,
                            IDataManagementService *,
                            SINT64 &,
                            rtnContextPtr *,
                            BOOLEAN ) >
         queryFunc = [ this ]( const CHAR *pCollectionName,
                               const BSONObj &selector,
                               const BSONObj &matcher,
                               const BSONObj &orderBy,
                               const BSONObj &hint,
                               SINT32 flags,
                               SINT64 numToSkip,
                               SINT64 numToReturn,
                               IDataManagementService *idms,
                               SINT64 &contextID,
                               rtnContextPtr *ppContext,
                               BOOLEAN enablePrefetch ) -> INT32 {
         INT32 rc = SDB_OK;
         contextID = ++curContextID;
         testExecutor executor;
         std::vector< BSONObj > vec;
         utilStringView csName( matcher.getStringField( RTN_STAT_COLLECTION_SPACE ) );
         utilStringView clShortName( matcher.getStringField( RTN_STAT_COLLECTION ) );
         utilStringView indexName( matcher.getStringField( RTN_STAT_IDX_INDEX ) );

         std::function< BOOLEAN( const collectionTest & ) > predicate;
         if ( csName == "" )
         {
            predicate = []( const collectionTest &cl ) -> BOOLEAN { return TRUE; };
         }
         else if ( clShortName == "" )
         {

            predicate = [ csName ]( const collectionTest &cl ) -> BOOLEAN {
               return utilStringView( cl._clFullName.substr( 0, cl._clFullName.find( '.' ) ) ) ==
                      csName;
            };
         }
         else
         {
            ossPoolString clFullName( matcher.getStringField( RTN_STAT_COLLECTION_SPACE ) );
            clFullName.push_back( '.' );
            clFullName.append( matcher.getStringField( RTN_STAT_COLLECTION ) );
            predicate = [ clFullName ]( const collectionTest &cl ) -> BOOLEAN {
               return clFullName == cl._clFullName;
            };
         }

         for ( auto it = dms.colls.begin(); it != dms.colls.end(); ++it )
         {
            if ( predicate( *( it->second ) ) )
            {
               if ( utilStringView( pCollectionName ) ==
                    utilStringView( DMS_STAT_COLLECTION_CL_NAME ) )
               {
                  vec.push_back( it->second->_clStatPtr->toBson() );
               }
               else if ( utilStringView( pCollectionName ) ==
                         utilStringView( DMS_STAT_INDEX_CL_NAME ) )
               {
                  for ( auto itIndex = it->second->_vecIndexStat.begin();
                        itIndex != it->second->_vecIndexStat.end(); ++itIndex )
                  {
                     vec.push_back( ( *itIndex )->toBson() );
                  }
               }
            }
         }

         contexts.emplace( contextID, std::move( vec ) );
         getMorePos.emplace( contextID, 0 );
         return rc;
      };

      std::function< INT32( SINT64, SINT32, rtnContextBuf & ) > getMoreFunc =
         [ this ]( SINT64 contextID, SINT32 maxNumToReturn, rtnContextBuf &buffObj ) -> INT32 {
         decltype( getMorePos )::iterator it = getMorePos.find( contextID );
         UINT32 pos = 0;
         if ( it != getMorePos.end() )
         {
            pos = it->second;
         }
         else
         {
            getMorePos.emplace( contextID, 0 );
         }

         if ( pos >= contexts[ contextID ].size() )
         {
            return SDB_DMS_EOC;
         }
         buffObj = rtnContextBuf( contexts[ contextID ][ pos ] );
         ++getMorePos[ contextID ];
         return SDB_OK;
      };

      std::function< INT32( INT32, const INT64 * ) > killContextFunc =
         [ this ]( INT32 numContexts, const INT64 *pContextIDs ) -> INT32 {
         contexts.erase( *pContextIDs );
         getMorePos.erase( *pContextIDs );
         return SDB_OK;
      };
   };

   INT32 dmsTest::openCL( IExecutor *executor,
                          const CHAR *fullName,
                          const dmsOpenCLOptions &o,
                          DATA_COLLECTION_PTR &ptr )
   {
      ptr = colls[ fullName ];
      return SDB_OK;
   }

   INT32 dmsTest::openCL( IExecutor *executor,
                          utilCLUniqueID cluid,
                          const dmsOpenCLOptions &o,
                          DATA_COLLECTION_PTR &ptr )
   {
      return SDB_OK;
   }
   // 测试使用集合名获取统计信息缓存
   TEST_F( rtn_object_stat_cache_test, base_get_stat )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      {
         const CHAR *clFullName = SAMPLE_CL_NAME_1;
         CONST_RTN_CL_STAT_PTR clStatPtr = nullptr;
         rc = statCache.getOrUpdateCLStat( &executor, clFullName, clStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }

      {
         const CHAR *clFullName = SAMPLE_CL_NAME_2;
         CONST_RTN_CL_STAT_PTR clStatPtr = nullptr;
         rc = statCache.getOrUpdateCLStat( &executor, clFullName, clStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }

      // 测试使用获取不存在的统计信息
      {
         const CHAR *clFullName = "cs.cl_not_exist";
         CONST_RTN_CL_STAT_PTR clStatPtr = nullptr;
         rc = statCache.getOrUpdateCLStat( &executor, clFullName, clStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         EXPECT_EQ( TRUE, clStatPtr == nullptr );
      }
   }

   // 测试通过CL名清除缓存
   TEST_F( rtn_object_stat_cache_test, base_remove_by_cl_name )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      {
         const CHAR *clFullName = SAMPLE_CL_NAME_1;
         CONST_RTN_CL_STAT_PTR clStatPtr = nullptr;
         rc = statCache.getOrUpdateCLStat( &executor, clFullName, clStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
         statCache.removeCLStat( clFullName );
         clStatPtr = statCache.getCLStat( clFullName );
         EXPECT_EQ( TRUE, clStatPtr == nullptr );
      }

      {
         const CHAR *clFullName = SAMPLE_CL_NAME_2;
         CONST_RTN_CL_STAT_PTR clStatPtr = nullptr;
         rc = statCache.getOrUpdateCLStat( &executor, clFullName, clStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
         statCache.removeCLStat( clFullName );
         clStatPtr = statCache.getCLStat( clFullName );
         EXPECT_EQ( TRUE, clStatPtr == nullptr );
      }
   }

   // 测试通过CS名清除缓存
   TEST_F( rtn_object_stat_cache_test, base_remove_by_cs_name )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      {
         const CHAR *clFullName = SAMPLE_CL_NAME_1;
         CONST_RTN_CL_STAT_PTR clStatPtr = nullptr;
         rc = statCache.getOrUpdateCLStat( &executor, clFullName, clStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }

      {
         const CHAR *clFullName = SAMPLE_CL_NAME_2;
         CONST_RTN_CL_STAT_PTR clStatPtr = nullptr;
         rc = statCache.getOrUpdateCLStat( &executor, clFullName, clStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }

      statCache.removeCLStatInCS( SAMPLE_CS_NAME );
      EXPECT_EQ( TRUE, statCache.getCLStat( SAMPLE_CL_NAME_1 ) == nullptr );
      EXPECT_EQ( TRUE, statCache.getCLStat( SAMPLE_CL_NAME_2 ) == nullptr );
   }

   // 加载单个集合统计信息到缓存中
   TEST_F( rtn_object_stat_cache_test, base_reload_cl_stats )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      {
         const CHAR *clFullName = SAMPLE_CL_NAME_1;
         rc = statCache.reloadCLStats( &executor, clFullName );
         ASSERT_EQ( SDB_OK, rc );
         CONST_RTN_CL_STAT_PTR clStatPtr = statCache.getCLStat( clFullName );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }

      {
         const CHAR *clFullName = SAMPLE_CL_NAME_2;
         rc = statCache.reloadCLStats( &executor, clFullName );
         CONST_RTN_CL_STAT_PTR clStatPtr = statCache.getCLStat( clFullName );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }

      {
         const CHAR *clFullName = "cs.cl_not_exist";
         rc = statCache.reloadCLStats( &executor, clFullName );
         CONST_RTN_CL_STAT_PTR clStatPtr = statCache.getCLStat( clFullName );
         EXPECT_EQ( TRUE, clStatPtr == nullptr );
      }
   }

   // 加载CS的所有集合统计信息到缓存中
   TEST_F( rtn_object_stat_cache_test, base_reload_cs_stats_1 )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;

      rc = statCache.reloadCSStats( &executor, SAMPLE_CS_NAME );
      ASSERT_EQ( SDB_OK, rc );
      {
         const CHAR *clFullName = SAMPLE_CL_NAME_1;
         CONST_RTN_CL_STAT_PTR clStatPtr = statCache.getCLStat( clFullName );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }

      {
         const CHAR *clFullName = SAMPLE_CL_NAME_2;
         CONST_RTN_CL_STAT_PTR clStatPtr = statCache.getCLStat( clFullName );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }
   }

   // 加载不存在的CS的所有集合统计信息到缓存中
   TEST_F( rtn_object_stat_cache_test, base_reload_cs_stats_2 )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;

      rc = statCache.reloadCSStats( &executor, "cs_not_exist" );
      ASSERT_EQ( SDB_OK, rc );
      {
         const CHAR *clFullName = SAMPLE_CL_NAME_1;
         CONST_RTN_CL_STAT_PTR clStatPtr = statCache.getCLStat( clFullName );
         EXPECT_EQ( TRUE, clStatPtr == nullptr );
      }

      {
         const CHAR *clFullName = SAMPLE_CL_NAME_2;
         CONST_RTN_CL_STAT_PTR clStatPtr = statCache.getCLStat( clFullName );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, clStatPtr == nullptr );
      }
   }

   // 加载所有集合统计信息到缓存中
   TEST_F( rtn_object_stat_cache_test, base_reload_all_cl_stats )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;

      rc = statCache.reloadAllStats( &executor );
      ASSERT_EQ( SDB_OK, rc );
      {
         const CHAR *clFullName = SAMPLE_CL_NAME_1;
         CONST_RTN_CL_STAT_PTR clStatPtr = statCache.getCLStat( clFullName );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }

      {
         const CHAR *clFullName = SAMPLE_CL_NAME_2;
         CONST_RTN_CL_STAT_PTR clStatPtr = statCache.getCLStat( clFullName );
         std::shared_ptr< collectionTest > expCL = dms.getCL( clFullName );
         EXPECT_EQ( TRUE, checkCollectionStat( clStatPtr, *expCL ) );
      }
   }
} // namespace engine
