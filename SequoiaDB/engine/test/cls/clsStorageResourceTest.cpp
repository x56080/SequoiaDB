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
*******************************************************************************/

#include "clsStorageResource.hpp"
#include "clsIndexInfo.hpp"
#include "clsStorageResourceAgent.hpp"
#include "dmsStatSUMgr.hpp"
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
   BSONObj generateClsIndexInfoBson( const clsIndexInfo &info )
   {
      BSONObjBuilder builder;
      builder.append( IXM_NAME_FIELD, info.getIndexName() );
      builder.append( IXM_INNERID_FIELD, utilGetIdxInnerID( info.getIdxInnerID() ) );
      builder.append( IXM_KEY_FIELD, info.getKeyPattern() );
      builder.append( DMS_ID_KEY_NAME, info.getOID() );
      builder.appendBool( IXM_UNIQUE_FIELD, info.isUnique() );
      builder.appendBool( IXM_ENFORCED_FIELD, info.isEnforced() );
      builder.appendBool( IXM_NOTNULL_FIELD, info.isNotNull() );
      builder.appendBool( IXM_NOTARRAY_FIELD, info.isNotArray() );
      builder.appendBool( IXM_FIELD_NAME_DROPDUPS, info.isDropDups() );
      return builder.obj();
   }

   constexpr const CHAR SAMPLE_CS_NAME[] = "cs";
   constexpr const CHAR SAMPLE_CL_NAME_1[] = "cs.cl1";
   constexpr const CHAR SAMPLE_CL_NAME_2[] = "cs.cl2";
   constexpr utilCSUniqueID SAMPLE_CSUID = 1;
   constexpr utilCLInnerID SAMPLE_CLIID_1 = 1;
   constexpr utilCLInnerID SAMPLE_CLIID_2 = 2;
   constexpr utilIdxInnerID SAMPLE_IDXIID_CL1_1 = 1;
   constexpr utilIdxInnerID SAMPLE_IDXIID_CL1_2 = 2;
   constexpr utilIdxInnerID SAMPLE_IDXIID_CL2_1 = 3;
   constexpr utilIdxInnerID SAMPLE_IDXIID_CL2_2 = 4;
   utilCLUniqueID SAMPLE_CLUID_1 = utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_1 );
   utilCLUniqueID SAMPLE_CLUID_2 = utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_2 );
   constexpr const CHAR SAMPLE_INDEX_NAME_CL1_1[] = "index1_1";
   constexpr const CHAR SAMPLE_INDEX_NAME_CL1_2[] = "index1_2";
   constexpr const CHAR SAMPLE_INDEX_NAME_CL2_1[] = "index2_1";
   constexpr const CHAR SAMPLE_INDEX_NAME_CL2_2[] = "index2_2";

   class collectionTest : public IDataCollection
   {
   public:
      collectionTest( const std::string clFullName,
                      utilCLUniqueID cluid,
                      const std::unordered_map< string, clsIndexInfo > &samples,
                      CONST_CLS_CL_STAT_PTR &&clStatPtr )
      : _infos( samples )
      , _clFullName( clFullName )
      , _cluid( cluid )
      , _clStatPtr( std::move( clStatPtr ) )
      {
      }

   public:
      virtual BOOLEAN isClosed() const override
      {
         return _closed;
      };
      virtual void close() override
      {
         _closed = TRUE;
      };

   public:
      virtual INT32 createIndex( IExecutor *executor,
                                 const dmsBuildIndexOptions &o,
                                 const bson::BSONObj &indexDef ) override
      {
         return SDB_OK;
      };

      virtual INT32 getMetaData( IExecutor *executor, bson::BSONObj &data ) override
      {
         data = BSON( FIELD_NAME_NAME << _clFullName << FIELD_NAME_CL_UNIQUEID << (INT64)_cluid );
         return SDB_OK;
      };

      virtual INT32 listIndex( IExecutor *executor,
                               ossPoolVector< bson::BSONObj > &indexes ) override
      {
         for ( std::unordered_map< string, clsIndexInfo >::const_iterator it = _infos.cbegin();
               it != _infos.cend(); ++it )
         {
            indexes.push_back( generateClsIndexInfoBson( it->second ) );
         }
         ++calledTimes;
         return 0;
      }

      virtual INT32 removeIndex( IExecutor *executor, const CHAR *indexName ) override
      {
         return SDB_OK;
      };

   public:
      virtual INT32 truncate( IExecutor *executor, const dmsTruncateCLOptions &o ) override
      {
         return SDB_OK;
      };

   public:
      virtual INT32 insertRecord( IExecutor *executor,
                                  const bson::BSONObj &record,
                                  const dmsInsertRecordOptions &o,
                                  utilInsertResult *result ) override
      {
         return SDB_OK;
      };
      virtual INT32 insertBatch( IExecutor *executor,
                                 const ossPoolVector< bson::BSONObj > &batch,
                                 const dmsInsertRecordOptions &o,
                                 utilInsertResult *result ) override
      {
         return SDB_OK;
      };

      virtual INT32 updateRecord( IExecutor *executor,
                                  const dmsRecordID &rid,
                                  IRecordUpdater *updater,
                                  const dmsUpdateRecordOptions &o,
                                  utilUpdateResult *result ) override
      {
         return SDB_OK;
      };

      virtual INT32 deleteRecord( IExecutor *executor,
                                  const dmsRecordID &rid,
                                  const dmsDeleteRecordOptions &o,
                                  utilDeleteResult *result ) override
      {
         return SDB_OK;
      };

   public:
      virtual INT32 scan( IExecutor *executor,
                          const dmsScanOptions &o,
                          DATA_CURSOR_PTR &cursor ) override
      {
         return SDB_OK;
      };

      virtual INT32 scanIndex( IExecutor *executor,
                               const CHAR *indexName,
                               const rtnPredicateList &predicate,
                               const dmsIndexScanOptions &o,
                               DATA_CURSOR_PTR &cursor ) override
      {
         return SDB_OK;
      };

      virtual INT32 getRecordCount( IExecutor *executor, UINT64 &count ) override
      {
         return SDB_OK;
      };

   public: /// lob
      virtual INT32 insertLobChunk( IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    UINT32 offset,
                                    UINT32 size,
                                    const CHAR *data ) override
      {
         return SDB_OK;
      };

      virtual INT32 readLobChunk( IExecutor *executor,
                                  const bson::OID &oid,
                                  UINT32 chunkId,
                                  UINT32 offset,
                                  UINT32 size,
                                  CHAR *data,
                                  UINT32 &readSize ) override
      {
         return SDB_OK;
      };

      virtual INT32 removeLobChunk( IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId ) override
      {
         return SDB_OK;
      };

      virtual INT32 updateLobChunk( IExecutor *executor,
                                    const bson::OID &oid,
                                    UINT32 chunkId,
                                    UINT32 offset,
                                    UINT32 size,
                                    const CHAR *data,
                                    BOOLEAN createIfNotExists ) override
      {
         return SDB_OK;
      };

      virtual INT32 truncateLobChunk( IExecutor *executor,
                                      const bson::OID &oid,
                                      UINT32 chunkId,
                                      UINT32 size,
                                      UINT32 &tsize ) override
      {
         return SDB_OK;
      };

      virtual INT32 listLobChunks( IExecutor *executor,
                                   const dmsListLobChunkOptions &o,
                                   DATA_CURSOR_PTR &cursor ) override
      {
         return SDB_OK;
      };

      virtual INT32 testLobChunk( IExecutor *executor,
                                  const bson::OID &oid,
                                  UINT32 chunkId,
                                  dmsLobChunkProfile *profile ) override
      {
         return SDB_OK;
      };

      const clsIndexInfo &getInfo( const CHAR *indexName )
      {
         return _infos[ indexName ];
      }

   public:
      INT32 calledTimes = 0;

   public:
      std::unordered_map< string, clsIndexInfo > _infos;
      std::string _clFullName;
      utilCLUniqueID _cluid = UTIL_UNIQUEID_NULL;
      CONST_CLS_CL_STAT_PTR _clStatPtr = CLS_DEFAULT_CL_STAT;
      BOOLEAN _closed = FALSE;
   };
   class dmsTest : public IDataManagementService
   {
   public:
      virtual INT32 openCL( IExecutor *executor,
                            const CHAR *fullName,
                            const dmsOpenCLOptions &o,
                            DATA_COLLECTION_PTR &ptr ) override;

      virtual INT32 openCL( IExecutor *executor,
                            utilCLUniqueID uniqueId,
                            const dmsOpenCLOptions &o,
                            DATA_COLLECTION_PTR &ptr ) override;

   public:
      std::shared_ptr< collectionTest > getCL( const CHAR *fullName )
      {
         return colls[ fullName ];
      }

   private:
      std::string _getNameByID( utilCLUniqueID cluid )
      {
         std::vector< std::pair< string, utilCLUniqueID > >::const_iterator it = std::find_if(
            clNameAndID.cbegin(), clNameAndID.cend(),
            [ &, cluid ]( const std::pair< string, utilCLUniqueID > &item ) -> BOOLEAN {
               return item.second == cluid;
            } );
         return it->first;
      }
      utilCLUniqueID _getIDByName( std::string clFullName )
      {
         std::vector< std::pair< string, utilCLUniqueID > >::const_iterator it = std::find_if(
            clNameAndID.cbegin(), clNameAndID.cend(),
            [ &, clFullName ]( const std::pair< string, utilCLUniqueID > &item ) -> BOOLEAN {
               return item.first.compare( clFullName ) == 0;
            } );
         return it->second;
      }

      clsIndexInfo buildClsIndexInfo( const CHAR *indexName,
                                      utilIdxInnerID idxInnerID,
                                      const OID &oid,
                                      BSONObj keyPattern,
                                      BOOLEAN isUnique,
                                      BOOLEAN isEnforced,
                                      BOOLEAN isNotNull,
                                      BOOLEAN isNotArray,
                                      BOOLEAN isDropDups,
                                      const CLS_INDEX_STAT_PTR &statPtr )
      {
         clsIndexInfo info;
         info.init( indexName, idxInnerID, oid, keyPattern, isUnique, isEnforced, isNotNull,
                    isNotArray, isDropDups, statPtr );
         return info;
      }

   public:
      std::vector< std::pair< string, utilCLUniqueID > > clNameAndID{
         { SAMPLE_CL_NAME_1, utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_1 ) },
         { SAMPLE_CL_NAME_2, utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_2 ) } };

      std::unordered_map< std::string, std::shared_ptr< collectionTest > > colls{
         { SAMPLE_CL_NAME_1,
           make_shared< collectionTest >(
              SAMPLE_CL_NAME_1,
              utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_1 ),
              std::unordered_map< std::string, clsIndexInfo >{
                 { SAMPLE_INDEX_NAME_CL1_1,
                   buildClsIndexInfo(
                      SAMPLE_INDEX_NAME_CL1_1,
                      utilBuildIdxUniqueID( SAMPLE_CSUID, 1 ),
                      OID::gen(),
                      BSON( "name" << 1 ),
                      false,
                      false,
                      false,
                      false,
                      false,
                      std::make_shared< clsIndexStat >( 1, 1, 1, 1, 1, 1, 1, 1 ) ) },
                 { SAMPLE_INDEX_NAME_CL1_2,
                   buildClsIndexInfo(
                      SAMPLE_INDEX_NAME_CL1_2,
                      utilBuildIdxUniqueID( SAMPLE_CSUID, 2 ),
                      OID::gen(),
                      BSON( "myid" << 1 ),
                      true,
                      false,
                      false,
                      false,
                      false,
                      std::make_shared< clsIndexStat >( 2, 2, 2, 2, 2, 2, 2, 2 ) ) } },
              std::make_shared< clsCLStat >( 1, 1, 1, 1, 1, 1 ) ) },
         { SAMPLE_CL_NAME_2,
           make_shared< collectionTest >(
              SAMPLE_CL_NAME_2,
              utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_2 ),
              std::unordered_map< std::string, clsIndexInfo >{
                 { SAMPLE_INDEX_NAME_CL2_1,
                   buildClsIndexInfo(
                      SAMPLE_INDEX_NAME_CL2_1,
                      utilBuildIdxUniqueID( SAMPLE_CSUID, 3 ),
                      OID::gen(),
                      BSON( "name" << -1 ),
                      false,
                      false,
                      false,
                      false,
                      false,
                      std::make_shared< clsIndexStat >( 3, 3, 3, 3, 3, 3, 3, 3 ) ) },
                 { SAMPLE_INDEX_NAME_CL2_2,
                   buildClsIndexInfo(
                      SAMPLE_INDEX_NAME_CL2_2,
                      utilBuildIdxUniqueID( SAMPLE_CSUID, 4 ),
                      OID::gen(),
                      BSON( "myid" << -1 ),
                      true,
                      false,
                      false,
                      false,
                      false,
                      std::make_shared< clsIndexStat >( 4, 4, 4, 4, 4, 4, 4, 4 ) ) } },
              std::make_shared< clsCLStat >( 2, 2, 2, 2, 2, 2 ) ) } };
   };

   class cls_storage_resource_test : public testing::Test
   {
   public:
      static void SetUpTestCase()
      {
         sdbEnablePD( "/opt/diaglog/sdb.log", 1, 1000 );
         setPDLevel( PDDEBUG );
      }

      static void TearDownTestCase() {}

      virtual void SetUp()
      {
         contexts.clear();
         getMorePos.clear();
      }

      INT64 assignContextID()
      {
         return ++curContextID;
      }

   public:
      dmsTest dms;
      std::unordered_map< INT64, std::vector< BSONObj > > contexts;
      std::unordered_map< INT64, UINT32 > getMorePos;
      INT64 curContextID = -1;
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
         const CHAR *csName = matcher.getStringField( DMS_STAT_COLLECTION_SPACE );
         const CHAR *clShortName = matcher.getStringField( DMS_STAT_COLLECTION );
         const CHAR *indexName = matcher.getStringField( DMS_STAT_IDX_INDEX );
         contextID = assignContextID();
         std::vector< BSONObj > vec;
         if ( utilStringView( pCollectionName ) == DMS_STAT_COLLECTION_CL_NAME )
         {
            if ( *csName )
            {
               if ( *clShortName )
               {

                  std::string clFullName;
                  clFullName.append( csName );
                  clFullName.append( "." );
                  clFullName.append( clShortName );
                  vec.push_back( dms.colls.at( clFullName )->_clStatPtr->toBson() );
               }
               else
               {
                  for ( decltype( dms.colls )::iterator it = dms.colls.begin();
                        it != dms.colls.end(); ++it )
                  {
                     if ( it->first.substr( 0, it->first.find_first_of( '.' ) ) ==
                          utilStringView( csName ) )
                     {
                        vec.push_back( it->second->_clStatPtr->toBson() );
                     }
                  }
               }
            }
            else
            {
               for ( decltype( dms.colls )::iterator it = dms.colls.begin(); it != dms.colls.end();
                     ++it )
               {
                  vec.push_back( it->second->_clStatPtr->toBson() );
               }
            }
         }
         else if ( utilStringView( pCollectionName ) == DMS_STAT_INDEX_CL_NAME )
         {
            if ( *csName )
            {
               if ( *clShortName )
               {
                  std::string clFullName;
                  clFullName.append( csName );
                  clFullName.append( "." );
                  clFullName.append( clShortName );
                  if ( *indexName )
                  {
                     vec.push_back(
                        dms.colls[ clFullName ]->_infos[ indexName ].getStat()->toBson() );
                  }
                  else
                  {
                     std::unordered_map< string, clsIndexInfo > &infos =
                        dms.colls[ clFullName ]->_infos;
                     for ( std::unordered_map< string, clsIndexInfo >::iterator it = infos.begin();
                           it != infos.end(); ++it )
                     {
                        BSONObjBuilder builder;
                        it->second.getStat()->toBson( builder );
                        builder.append( DMS_STAT_IDX_INDEX, it->second.getIndexName() );
                        vec.push_back( builder.obj() );
                     }
                  }
               }
               else
               {
                  for ( decltype( dms.colls )::iterator it = dms.colls.begin();
                        it != dms.colls.end(); ++it )
                  {
                     if ( it->first.substr( 0, it->first.find_first_of( '.' ) ) ==
                          utilStringView( csName ) )
                     {
                        std::unordered_map< string, clsIndexInfo > &infos =
                           dms.colls[ it->first ]->_infos;
                        for ( std::unordered_map< string, clsIndexInfo >::iterator itInfo =
                                 infos.begin();
                              itInfo != infos.end(); ++itInfo )
                        {
                           BSONObjBuilder builder;
                           itInfo->second.getStat()->toBson( builder );
                           builder.append( DMS_STAT_IDX_INDEX, itInfo->second.getIndexName() );
                           vec.push_back( builder.obj() );
                        }
                     }
                  }
               }
            }
            else
            {
               for ( decltype( dms.colls )::iterator it = dms.colls.begin(); it != dms.colls.end();
                     ++it )
               {
                  std::unordered_map< string, clsIndexInfo > &infos =
                     dms.colls[ it->first ]->_infos;
                  for ( std::unordered_map< string, clsIndexInfo >::iterator itInfo = infos.begin();
                        itInfo != infos.end(); ++itInfo )
                  {
                     BSONObjBuilder builder;
                     itInfo->second.getStat()->toBson( builder );
                     builder.append( DMS_STAT_IDX_INDEX, itInfo->second.getIndexName() );
                     vec.push_back( builder.obj() );
                  }
               }
            }
         }
         contexts.emplace( contextID, std::move( vec ) );
         return SDB_OK;
      };

      std::function< INT32( SINT64, SINT32, rtnContextBuf & ) > getMoreFunc =
         [ this ]( SINT64 contextID, SINT32 maxNumToReturn, rtnContextBuf &buffObj ) -> INT32 {
         decltype( getMorePos )::iterator it = getMorePos.find( contextID );
         if ( it == getMorePos.end() )
         {
            getMorePos.emplace( contextID, 0 );
         }
         else
         {
            ++it->second;
         }
         UINT32 pos = getMorePos[ contextID ];
         if ( pos >= contexts[ contextID ].size() )
         {
            return SDB_DMS_EOC;
         }
         buffObj = rtnContextBuf( contexts[ contextID ][ pos ] );
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
      std::string fullName = _getNameByID( cluid );
      ptr = colls[ fullName ];
      return SDB_OK;
   }
   // 测试分别使用集合名和CL Unique ID更新元数据缓存
   TEST_F( cls_storage_resource_test, base_update_info )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      std::unique_ptr< clsStorageResourceAgent > agent =
         newClsStorageResourceAgentImpl( &dms, queryFunc, getMoreFunc, killContextFunc );
      clsStorageResource res;
      res.init( std::move( agent ) );
      CONST_CLS_INDEX_INFO_SET_PTR indexSetPtr;
      rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
      ASSERT_EQ( SDB_OK, rc );
      ASSERT_EQ( indexSetPtr->getVec().size(), 2 );

      {
         const CLS_INDEX_INFO_PTR info = indexSetPtr->get( SAMPLE_INDEX_NAME_CL1_1 );
         ASSERT_EQ( info != nullptr, TRUE );
         ASSERT_STREQ( info->getIndexName(), SAMPLE_INDEX_NAME_CL1_1 );
      }
      {
         const CLS_INDEX_INFO_PTR info = indexSetPtr->get( SAMPLE_INDEX_NAME_CL1_2 );
         ASSERT_EQ( info != nullptr, TRUE );
         ASSERT_STREQ( info->getIndexName(), SAMPLE_INDEX_NAME_CL1_2 );
      }

      rc = res.getOrUpdateCLIndexSet(
         &executor, utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_2 ), indexSetPtr );
      ASSERT_EQ( SDB_OK, rc );
      ASSERT_EQ( indexSetPtr->getVec().size(), 2 );

      {
         const CLS_INDEX_INFO_PTR info = indexSetPtr->get( SAMPLE_INDEX_NAME_CL2_1 );
         ASSERT_EQ( info != nullptr, TRUE );
         ASSERT_STREQ( info->getIndexName(), SAMPLE_INDEX_NAME_CL2_1 );
      }
      {
         const CLS_INDEX_INFO_PTR info = indexSetPtr->get( SAMPLE_INDEX_NAME_CL2_2 );
         ASSERT_EQ( info != nullptr, TRUE );
         ASSERT_STREQ( info->getIndexName(), SAMPLE_INDEX_NAME_CL2_2 );
      }
   }

   // 测试多线程同时更新缓存，仅会向dms发起一次请求
   TEST_F( cls_storage_resource_test, base_update_2 )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      std::unique_ptr< clsStorageResourceAgent > agent =
         newClsStorageResourceAgentImpl( &dms, queryFunc, getMoreFunc, killContextFunc );
      clsStorageResource res;
      res.init( std::move( agent ) );
      constexpr INT32 threadCount = 10;
      auto threadFun = [ & ]( clsStorageResource &res, const CHAR *clFullName ) {
         CONST_CLS_INDEX_INFO_SET_PTR indexSetPtr;
         rc = res.getOrUpdateCLIndexSet( &executor, clFullName, indexSetPtr );
         ASSERT_EQ( SDB_OK, rc );
         const ossPoolVector< CLS_INDEX_INFO_PTR > &indexes = indexSetPtr->getVec();
         ASSERT_EQ( indexes.size(), 2 );
      };
      std::thread ths[ threadCount ];
      for ( UINT32 i = 0; i < threadCount; ++i )
      {
         std::thread th( threadFun, std::ref( res ), SAMPLE_CL_NAME_1 );
         ths[ i ] = std::move( th );
      }
      for ( UINT32 i = 0; i < threadCount; ++i )
      {
         ths[ i ].join();
      }
      dmsOpenCLOptions o;
      DATA_COLLECTION_PTR ptr;
      std::shared_ptr< collectionTest > coll = dms.getCL( SAMPLE_CL_NAME_1 );
      ASSERT_EQ( coll->calledTimes, 1 );
   }

   // 测试通过CS/CL名清除缓存
   TEST_F( cls_storage_resource_test, base_remove_by_name )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      std::unique_ptr< clsStorageResourceAgent > agent =
         newClsStorageResourceAgentImpl( &dms, queryFunc, getMoreFunc, killContextFunc );
      clsStorageResource res;
      res.init( std::move( agent ) );
      CONST_CLS_INDEX_INFO_SET_PTR indexSetPtr;
      indexSetPtr = res.getCLIndexSet( SAMPLE_CL_NAME_1 );
      ASSERT_EQ( TRUE, indexSetPtr == nullptr );
      rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
      ASSERT_EQ( SDB_OK, rc );
      ASSERT_EQ( indexSetPtr->getVec().size(), 2 );
      res.removeCLMetaCache( SAMPLE_CL_NAME_1 );
      ASSERT_EQ( indexSetPtr->getVec().size(), 2 );
      indexSetPtr = res.getCLIndexSet( SAMPLE_CL_NAME_1 );
      ASSERT_EQ( TRUE, indexSetPtr == nullptr );

      rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
      ASSERT_EQ( SDB_OK, rc );
      rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_2, indexSetPtr );
      ASSERT_EQ( SDB_OK, rc );
      res.removeCSMetaCache( SAMPLE_CS_NAME );
      indexSetPtr = res.getCLIndexSet( SAMPLE_CL_NAME_1 );
      ASSERT_EQ( TRUE, indexSetPtr == nullptr );
      indexSetPtr = res.getCLIndexSet( SAMPLE_CL_NAME_2 );
      ASSERT_EQ( TRUE, indexSetPtr == nullptr );
   }

   // 测试通过CS/CL Unique ID清除缓存
   TEST_F( cls_storage_resource_test, base_remove_by_uid )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      std::unique_ptr< clsStorageResourceAgent > agent =
         newClsStorageResourceAgentImpl( &dms, queryFunc, getMoreFunc, killContextFunc );
      clsStorageResource res;
      res.init( std::move( agent ) );
      CONST_CLS_INDEX_INFO_SET_PTR indexSetPtr;
      indexSetPtr = res.getCLIndexSet( SAMPLE_CL_NAME_1 );
      ASSERT_EQ( TRUE, indexSetPtr == nullptr );
      rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
      ASSERT_EQ( SDB_OK, rc );
      ASSERT_EQ( indexSetPtr->getVec().size(), 2 );
      res.removeCLMetaCache( utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_1 ) );
      ASSERT_EQ( indexSetPtr->getVec().size(), 2 );
      indexSetPtr = res.getCLIndexSet( SAMPLE_CL_NAME_1 );
      ASSERT_EQ( TRUE, indexSetPtr == nullptr );

      rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
      ASSERT_EQ( SDB_OK, rc );
      rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_2, indexSetPtr );
      ASSERT_EQ( SDB_OK, rc );
      res.removeCSMetaCache( SAMPLE_CSUID );
      indexSetPtr = res.getCLIndexSet( SAMPLE_CL_NAME_1 );
      ASSERT_EQ( TRUE, indexSetPtr == nullptr );
      indexSetPtr = res.getCLIndexSet( SAMPLE_CL_NAME_2 );
      ASSERT_EQ( TRUE, indexSetPtr == nullptr );
   }

   TEST_F( cls_storage_resource_test, base_update_cl_stat )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      clsStorageResource res;
      std::unique_ptr< clsStorageResourceAgent > agent =
         newClsStorageResourceAgentImpl( &dms, queryFunc, getMoreFunc, killContextFunc );
      res.init( std::move( agent ) );
      {
         CONST_CLS_CL_STAT_PTR clStatPtr;
         rc = res.getOrUpdateCLStat( &executor, SAMPLE_CL_NAME_1, clStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         EXPECT_EQ( clStatPtr->getCreateTime(), 1 );
         clStatPtr.reset();
         clStatPtr = res.getCLStat( SAMPLE_CL_NAME_1 );
         ASSERT_EQ( TRUE, clStatPtr != nullptr );
         EXPECT_EQ( clStatPtr->getCreateTime(), 1 );
         clStatPtr.reset();
         rc = res.getOrUpdateCLStat( &executor, SAMPLE_CLUID_2, clStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         EXPECT_EQ( clStatPtr->getCreateTime(), 2 );
         clStatPtr.reset();
         clStatPtr = res.getCLStat( SAMPLE_CLUID_2 );
         ASSERT_EQ( TRUE, clStatPtr != nullptr );
         EXPECT_EQ( clStatPtr->getCreateTime(), 2 );
      }
   }

   TEST_F( cls_storage_resource_test, base_update_index_stat )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      clsStorageResource res;

      std::unique_ptr< clsStorageResourceAgent > agent =
         newClsStorageResourceAgentImpl( &dms, queryFunc, getMoreFunc, killContextFunc );
      res.init( std::move( agent ) );
      {
         CONST_CLS_INDEX_STAT_PTR indexStatPtr;
         rc = res.getOrUpdateIndexStat( &executor, SAMPLE_CL_NAME_1, SAMPLE_INDEX_NAME_CL1_1,
                                        indexStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         EXPECT_EQ( indexStatPtr->getCreateTime(), 1 );
         indexStatPtr.reset();
         indexStatPtr = res.getIndexStat( SAMPLE_CL_NAME_1, SAMPLE_INDEX_NAME_CL1_1 );
         ASSERT_EQ( TRUE, indexStatPtr != nullptr );
         EXPECT_EQ( indexStatPtr->getCreateTime(), 1 );
         indexStatPtr.reset();
         rc = res.getOrUpdateIndexStat( &executor, SAMPLE_CLUID_2, SAMPLE_IDXIID_CL2_1,
                                        indexStatPtr );
         ASSERT_EQ( SDB_OK, rc );
         EXPECT_EQ( indexStatPtr->getCreateTime(), 3 );
         indexStatPtr.reset();
         indexStatPtr = res.getIndexStat( SAMPLE_CLUID_2, SAMPLE_IDXIID_CL2_1 );
         ASSERT_EQ( TRUE, indexStatPtr != nullptr );
         EXPECT_EQ( indexStatPtr->getCreateTime(), 3 );
      }
      {
         res.invalidateStorageCache();
         CONST_CLS_INDEX_INFO_SET_PTR indexSetPtr;
         rc = res.getOrUpdateCLIndexStats( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
         ASSERT_EQ( SDB_OK, rc );
         {
            CONST_CLS_INDEX_STAT_PTR indexStatPtr;
            indexStatPtr = res.getIndexStat( SAMPLE_CL_NAME_1, SAMPLE_INDEX_NAME_CL1_1 );
            ASSERT_EQ( TRUE, indexStatPtr != nullptr );
            EXPECT_EQ( indexStatPtr->getCreateTime(), 1 );
         }
         {
            CONST_CLS_INDEX_STAT_PTR indexStatPtr;
            indexStatPtr = res.getIndexStat( SAMPLE_CL_NAME_1, SAMPLE_INDEX_NAME_CL1_2 );
            ASSERT_EQ( TRUE, indexStatPtr != nullptr );
            EXPECT_EQ( indexStatPtr->getCreateTime(), 2 );
         }
      }
      {
         res.invalidateStorageCache();
         CONST_CLS_INDEX_INFO_SET_PTR indexSetPtr;
         rc = res.getOrUpdateCLIndexStats( &executor, SAMPLE_CLUID_2, indexSetPtr );
         ASSERT_EQ( SDB_OK, rc );
         {
            CONST_CLS_INDEX_STAT_PTR indexStatPtr;
            indexStatPtr = res.getIndexStat( SAMPLE_CLUID_2, SAMPLE_IDXIID_CL2_1 );
            ASSERT_EQ( TRUE, indexStatPtr != nullptr );
            EXPECT_EQ( indexStatPtr->getCreateTime(), 3 );
         }
         {
            CONST_CLS_INDEX_STAT_PTR indexStatPtr;
            indexStatPtr = res.getIndexStat( SAMPLE_CLUID_2, SAMPLE_IDXIID_CL2_2 );
            ASSERT_EQ( TRUE, indexStatPtr != nullptr );
            EXPECT_EQ( indexStatPtr->getCreateTime(), 4 );
         }
      }
   }
   TEST_F( cls_storage_resource_test, base_upsert_cl_stat )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      clsStorageResource res;
      std::unique_ptr< clsStorageResourceAgent > agent =
         newClsStorageResourceAgentImpl( &dms, queryFunc, getMoreFunc, killContextFunc );
      res.init( std::move( agent ) );
      {
         clsCLStat clStat( 5, 5, 5, 5, 5, 5 );
         BSONObj obj = clStat.toBson();
         rc = res.upsertCLStat( &executor, SAMPLE_CL_NAME_1, SAMPLE_CLUID_1, obj );
         ASSERT_EQ( SDB_OK, rc );
         CONST_CLS_CL_STAT_PTR clStatPtr = nullptr;
         clStatPtr = res.getCLStat( SAMPLE_CL_NAME_1 );
         ASSERT_EQ( TRUE, clStatPtr != nullptr );
         EXPECT_EQ( clStatPtr->getCreateTime(), 5 );
         clStatPtr = res.getCLStat( SAMPLE_CLUID_1 );
         ASSERT_EQ( TRUE, clStatPtr != nullptr );
         EXPECT_EQ( clStatPtr->getCreateTime(), 5 );
      }
   }

   TEST_F( cls_storage_resource_test, base_upsert_index_stat )
   {
      INT32 rc = SDB_OK;
      testExecutor executor;
      clsStorageResource res;
      std::unique_ptr< clsStorageResourceAgent > agent =
         newClsStorageResourceAgentImpl( &dms, queryFunc, getMoreFunc, killContextFunc );
      res.init( std::move( agent ) );
      {
         clsIndexStat indexStat( 5, 5, 5, 5, 5, 5, 5, 5 );
         BSONObj obj = indexStat.toBson();
         rc = res.upsertIndexStat( &executor, SAMPLE_CL_NAME_1, SAMPLE_CLUID_1,
                                   SAMPLE_INDEX_NAME_CL1_1, obj );
         ASSERT_EQ( SDB_OK, rc );
         CONST_CLS_INDEX_STAT_PTR indexStatPtr = nullptr;
         indexStatPtr = res.getIndexStat( SAMPLE_CL_NAME_1, SAMPLE_INDEX_NAME_CL1_1 );
         ASSERT_EQ( TRUE, indexStatPtr != nullptr );
         EXPECT_EQ( indexStatPtr->getCreateTime(), 5 );
         indexStatPtr = res.getIndexStat( SAMPLE_CLUID_1, SAMPLE_IDXIID_CL1_1 );
         ASSERT_EQ( TRUE, indexStatPtr != nullptr );
         EXPECT_EQ( indexStatPtr->getCreateTime(), 5 );
      }
   }
} // namespace engine
