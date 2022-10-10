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
#include "interface/IDataCollection.h"
#include "interface/IDataManagementService.h"
#include "msgDef.h"
#include "../testDef.h"
#include "sdbInterface.hpp"
#include "utilStringView.hpp"
#include "utilUniqueID.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <unordered_map>
namespace engine
{
BSONObj generateClsIndexInfoBson( const clsIndexInfo &info )
{
   BSONObjBuilder builder;
   builder.append( IXM_NAME_FIELD, info.getIndexName() );
   builder.append( IXM_INNERID_FIELD,
                   utilGetIdxInnerID( info.getIdxInnerID() ) );
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

class collectionTest : public IDataCollection
{
public:
   collectionTest( const std::string clFullName,
                   utilCLUniqueID cluid,
                   const std::unordered_map< string, clsIndexInfo > &samples )
   : _samples( samples ), _clFullName( clFullName ), _cluid( cluid )
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

   virtual INT32 getMetaData( IExecutor *executor,
                              bson::BSONObj &data ) override
   {
      data = BSON( FIELD_NAME_NAME << _clFullName << FIELD_NAME_CL_UNIQUEID
                                   << (INT64)_cluid );
      return SDB_OK;
   };

   virtual INT32 listIndex( IExecutor *executor,
                            ossPoolVector< bson::BSONObj > &indexes ) override
   {
      for ( std::unordered_map< string, clsIndexInfo >::const_iterator it =
               _samples.cbegin();
            it != _samples.cend();
            ++it )
      {
         indexes.push_back( generateClsIndexInfoBson( it->second ) );
      }
      ++calledTimes;
      return 0;
   }

   virtual INT32 removeIndex( IExecutor *executor,
                              const CHAR *indexName ) override
   {
      return SDB_OK;
   };

public:
   virtual INT32 truncate( IExecutor *executor,
                           const dmsTruncateCLOptions &o ) override
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
      return _samples[ indexName ];
   }

public:
   INT32 calledTimes = 0;

private:
   std::unordered_map< string, clsIndexInfo > _samples;
   std::string _clFullName;
   utilCLUniqueID _cluid;
   BOOLEAN _closed;
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
      return _colls[ fullName ];
   }

private:
   std::string _getNameByID( utilCLUniqueID cluid )
   {
      std::vector< std::pair< string, utilCLUniqueID > >::const_iterator it =
         std::find_if(
            _clNameAndID.cbegin(),
            _clNameAndID.cend(),
            [ &, cluid ]( const std::pair< string, utilCLUniqueID > &item )
               -> BOOLEAN { return item.second == cluid; } );
      return it->first;
   }
   utilCLUniqueID _getIDByName( std::string clFullName )
   {
      std::vector< std::pair< string, utilCLUniqueID > >::const_iterator it =
         std::find_if(
            _clNameAndID.cbegin(),
            _clNameAndID.cend(),
            [ &, clFullName ]( const std::pair< string, utilCLUniqueID > &item )
               -> BOOLEAN { return item.first.compare( clFullName ) == 0; } );
      return it->second;
   }

private:
   std::vector< std::pair< string, utilCLUniqueID > > _clNameAndID{
      { SAMPLE_CL_NAME_1, utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_1 ) },
      { SAMPLE_CL_NAME_2,
        utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_2 ) } };

   std::unordered_map< std::string, std::shared_ptr< collectionTest > > _colls{
      { SAMPLE_CL_NAME_1,
        make_shared< collectionTest >(
           SAMPLE_CL_NAME_1,
           utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_1 ),
           std::unordered_map< std::string, clsIndexInfo >{
              { "index1",
                clsIndexInfo( "index1",
                              utilBuildIdxUniqueID( SAMPLE_CSUID, 1 ),
                              OID::gen(),
                              BSON( "name" << 1 ),
                              false,
                              false,
                              false,
                              false,
                              false ) },
              { "index2",
                clsIndexInfo( "index2",
                              utilBuildIdxUniqueID( SAMPLE_CSUID, 2 ),
                              OID::gen(),
                              BSON( "myid" << 1 ),
                              true,
                              false,
                              false,
                              false,
                              false ) } } ) },
      { SAMPLE_CL_NAME_2,
        make_shared< collectionTest >(
           SAMPLE_CL_NAME_2,
           utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_2 ),
           std::unordered_map< std::string, clsIndexInfo >{
              { "index3",
                clsIndexInfo( "index3",
                              utilBuildIdxUniqueID( SAMPLE_CSUID, 3 ),
                              OID::gen(),
                              BSON( "name" << -1 ),
                              false,
                              false,
                              false,
                              false,
                              false ) },
              { "index4",
                clsIndexInfo( "index4",
                              utilBuildIdxUniqueID( SAMPLE_CSUID, 4 ),
                              OID::gen(),
                              BSON( "myid" << -1 ),
                              true,
                              false,
                              false,
                              false,
                              false ) } } ) } };
};

class cls_storage_resource_test : public testing::Test
{
public:
   static void SetUpTestCase()
   {
      sdbEnablePD( "/opt/diaglog/sdb.log", 1, 1000 );
      setPDLevel( PDDEBUG );
   }

   static void TearDownTestCase()
   {
   }

   virtual void SetUp()
   {
   }
};

INT32 dmsTest::openCL( IExecutor *executor,
                       const CHAR *fullName,
                       const dmsOpenCLOptions &o,
                       DATA_COLLECTION_PTR &ptr )
{
   ptr = _colls[ fullName ];
   return SDB_OK;
}

INT32 dmsTest::openCL( IExecutor *executor,
                       utilCLUniqueID cluid,
                       const dmsOpenCLOptions &o,
                       DATA_COLLECTION_PTR &ptr )
{
   std::string fullName = _getNameByID( cluid );
   ptr = _colls[ fullName ];
   return SDB_OK;
}
// 测试分别使用集合名和CL Unique ID更新缓存
TEST_F( cls_storage_resource_test, base_update )
{
   INT32 rc = SDB_OK;
   testExecutor executor;
   dmsTest dms;
   std::unique_ptr< clsStorageResourceAgent > agent =
      newClsStorageResourceAgentImpl( &dms );
   clsStorageResource res;
   res.init( std::move( agent ) );
   clsIndexInfoSetPtr indexSetPtr;
   rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_OK, rc );
   ASSERT_EQ( indexSetPtr->getAll().size(), 2 );

   {
      const clsIndexInfo *info = indexSetPtr->get( "index1" );
      ASSERT_EQ( info != nullptr, TRUE );
      ASSERT_STREQ( info->getIndexName(), "index1" );
   }
   {
      const clsIndexInfo *info = indexSetPtr->get( "index2" );
      ASSERT_EQ( info != nullptr, TRUE );
      ASSERT_STREQ( info->getIndexName(), "index2" );
   }

   rc = res.getOrUpdateCLIndexSet(
      &executor,
      utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_2 ),
      indexSetPtr );
   ASSERT_EQ( SDB_OK, rc );
   ASSERT_EQ( indexSetPtr->getAll().size(), 2 );

   {
      const clsIndexInfo *info = indexSetPtr->get( "index3" );
      ASSERT_EQ( info != nullptr, TRUE );
      ASSERT_STREQ( info->getIndexName(), "index3" );
   }
   {
      const clsIndexInfo *info = indexSetPtr->get( "index4" );
      ASSERT_EQ( info != nullptr, TRUE );
      ASSERT_STREQ( info->getIndexName(), "index4" );
   }
}

// 测试多线程同时更新缓存，仅会向dms发起一次请求
TEST_F( cls_storage_resource_test, base_update_2 )
{
   INT32 rc = SDB_OK;
   testExecutor executor;
   dmsTest dms;
   std::unique_ptr< clsStorageResourceAgent > agent =
      newClsStorageResourceAgentImpl( &dms );
   clsStorageResource res;
   res.init( std::move( agent ) );
   clsIndexInfoSetPtr indexSetPtr;
   constexpr INT32 threadCount = 10;
   auto threadFun = [ & ]( clsStorageResource &res, const CHAR *clFullName ) {
      clsIndexInfoSetPtr indexSetPtr;
      rc = res.getOrUpdateCLIndexSet( &executor, clFullName, indexSetPtr );
      ASSERT_EQ( SDB_OK, rc );
      const ossPoolVector< clsIndexInfo > &indexes = indexSetPtr->getAll();
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
   dmsTest dms;
   std::unique_ptr< clsStorageResourceAgent > agent =
      newClsStorageResourceAgentImpl( &dms );
   clsStorageResource res;
   res.init( std::move( agent ) );
   clsIndexInfoSetPtr indexSetPtr;
   rc = res.getCLIndexSet( SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc );
   rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_OK, rc );
   ASSERT_EQ( indexSetPtr->getAll().size(), 2 );
   res.removeCLMetaCache( SAMPLE_CL_NAME_1 );
   ASSERT_EQ( indexSetPtr->getAll().size(), 2 );
   rc = res.getCLIndexSet( SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc );

   rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_OK, rc );
   rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_2, indexSetPtr );
   ASSERT_EQ( SDB_OK, rc );
   res.removeCSMetaCache( SAMPLE_CS_NAME );
   rc = res.getCLIndexSet( SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc );
   rc = res.getCLIndexSet( SAMPLE_CL_NAME_2, indexSetPtr );
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc );
}

// 测试通过CS/CL Unique ID清除缓存
TEST_F( cls_storage_resource_test, base_remove_by_uid )
{
   INT32 rc = SDB_OK;
   testExecutor executor;
   dmsTest dms;
   std::unique_ptr< clsStorageResourceAgent > agent =
      newClsStorageResourceAgentImpl( &dms );
   clsStorageResource res;
   res.init( std::move( agent ) );
   clsIndexInfoSetPtr indexSetPtr;
   rc = res.getCLIndexSet( SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc );
   rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_OK, rc );
   ASSERT_EQ( indexSetPtr->getAll().size(), 2 );
   res.removeCLMetaCache( utilBuildCLUniqueID( SAMPLE_CSUID, SAMPLE_CLIID_1 ) );
   ASSERT_EQ( indexSetPtr->getAll().size(), 2 );
   rc = res.getCLIndexSet( SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc );

   rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_OK, rc );
   rc = res.getOrUpdateCLIndexSet( &executor, SAMPLE_CL_NAME_2, indexSetPtr );
   ASSERT_EQ( SDB_OK, rc );
   res.removeCSMetaCache( SAMPLE_CSUID );
   rc = res.getCLIndexSet( SAMPLE_CL_NAME_1, indexSetPtr );
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc );
   rc = res.getCLIndexSet( SAMPLE_CL_NAME_2, indexSetPtr );
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc );
}
} // namespace engine
