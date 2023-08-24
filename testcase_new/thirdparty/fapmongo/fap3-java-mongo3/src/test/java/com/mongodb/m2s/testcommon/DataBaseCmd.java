package com.mongodb.m2s.testcommon;

import com.mongodb.CursorType;
import com.mongodb.client.*;
import com.mongodb.client.model.*;
import org.bson.BsonArray;
import org.bson.BsonDocument;
import org.bson.BsonDouble;
import org.bson.Document;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.TimeUnit;

/**
 * @Descreption 构造数据库命令,数据库命令参考4.4版本mongodb官方文档
 * @Author
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class DataBaseCmd {
    private String databaseName = "databaseCmd_test";
    private String collectionName = "collectionCmd_test";
    private MongoClient mongoClient = null;
    private MongoDatabase adminClient = null;
    private String mongoVerson = null;

    public DataBaseCmd( MongoClient mongoClients ) {
        this.mongoClient = mongoClients;
        this.mongoVerson = CommLib.getMongoDBVersion( mongoClient );
        this.adminClient = mongoClient.getDatabase( "admin" );
    }

    private void setUp() {
        mongoClient.getDatabase( databaseName ).drop();
    }

    /**
     * 构造aggregation类型数据库命令 包含以下命令： aggregate count distinct mapReduce
     */
    public void runAggregationCommands() {
        setUp();
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        database.createCollection( collectionName );
        ArrayList< Document > list = new ArrayList<>();
        for ( int i = 0; i < 50; i++ ) {
            list.add( new Document( "a", i ).append( "b", "str" + i )
                    .append( "c", Arrays.asList( i + 1, i + 2, i + 3 ) )
                    .append( "d", i ).append( "count", "str" + i ) );
        }
        MongoCollection< Document > cl = database
                .getCollection( collectionName );
        cl.insertMany( list );
        cl.createIndex( new Document( "a", 1 ) );
        // aggregate
        cl.aggregate( Arrays.asList( Aggregates.unwind( "$c" ),
                Aggregates.match( Filters.and( Filters.gt( "a", 0 ),
                        Filters.or( Filters.regex( "b", "str\\d+" ),
                                Filters.exists( "c" ),
                                Filters.mod( "d", 1, 1 ) ) ) ),
                Aggregates.group( "$d", Accumulators.sum( "sum", "$a" ),
                        Accumulators.avg( "avg", "$a" ),
                        Accumulators.first( "first", "$a" ),
                        Accumulators.last( "last", "$a" ),
                        Accumulators.push( "push", "$a" ),
                        Accumulators.max( "max", "$a" ),
                        Accumulators.min( "min", "$a" ),
                        Accumulators.addToSet( "addToSet", "$a" ),
                        Accumulators.stdDevPop( "a", "$a" ),
                        Accumulators.stdDevSamp( "stdDevSamp", "$a" ) ),
                Aggregates.sort( Sorts.ascending( "d" ) ),
                Aggregates.limit( 10 ),
                Aggregates.project( Projections.fields(
                        Projections.include( "a", "b", "c", "d" ),
                        Projections.excludeId() ) ),
                Aggregates.sample( 100 ), Aggregates.skip( 1 ),
                Aggregates.out( "outDB" ) ) ).allowDiskUse( true )
                .batchSize( 1 ).bypassDocumentValidation( true )
                .maxTime( 1000, TimeUnit.SECONDS )
                .maxAwaitTime( 1000, TimeUnit.SECONDS ).useCursor( false )
                .into( new ArrayList<>() );

        // count
        Document document = new Document( "count", collectionName )
                .append( "query",
                        new Document( "a", new Document( "$gt", 0 ) ) )
                .append( "limit", 10 ).append( "skip", 1 )
                .append( "hint", new Document( "a", 1 ) )
                .append( "maxTimeMS", 1000 )
                .append( "readConcern", new Document( "level", "local" ) );
        if ( CommLib.compareVersion( mongoVerson, "3.4" ) >= 0 ) {
            document.append( "collation", new Document( "locale", "en_US" ) );
        }
        adminClient.runCommand( document );

        // distinct
        document = new Document( "distinct", collectionName )
                .append( "key", "a" )
                .append( "query",
                        new Document( "a", new Document( "$gt", 0 ) ) )
                .append( "readConcern", new Document( "level", "local" ) );
        if ( CommLib.compareVersion( mongoVerson, "3.4" ) >= 0 ) {
            document.append( "collation", new Document( "locale", "en_US" ) );
        }
        if ( CommLib.compareVersion( mongoVerson, "4.4" ) >= 0 ) {
            document.append( "comment", "comment" );
        }
        adminClient.runCommand( document );

        // mapReduce
        cl.mapReduce( "function() { emit(this.a, this.b); }",
                "function(key, values) { return Array.sum(values); }" )
                .action( MapReduceAction.REPLACE ).batchSize( 1 )
                .bypassDocumentValidation( true )
                .maxTime( 1000, TimeUnit.SECONDS )
                .bypassDocumentValidation( true )
                .collectionName( collectionName ).databaseName( databaseName )
                .limit( 10 ).nonAtomic( false ).sharded( true )
                .sort( new Document( "a", 1 ) )
                .filter( Filters.and( Filters.gt( "a", 0 ),
                        Filters.or( Filters.regex( "b", "str\\d+" ),
                                Filters.exists( "c" ),
                                Filters.mod( "d", 1, 1 ) ) ) )
                .scope( new Document( "scope", "scope" ) ).jsMode( true )
                .verbose( true ).into( new ArrayList<>() );
        tearDown();
    }

    /**
     * 构造geospatial类型数据库命令 包含以下命令： geoSearch
     */
    public void runGeoSpatialCommands() {
        // geoSearch is not supported in sharded cluster
        if ( !CommLib.isSharded( mongoClient ) ) {
            setUp();
            MongoDatabase database = mongoClient.getDatabase( databaseName );
            MongoCollection< Document > cl = database
                    .getCollection( collectionName );
            BsonArray location1 = new BsonArray();
            location1.add( new BsonDouble( 34.55 ) );
            location1.add( new BsonDouble( -34.55 ) );
            Document location2 = new Document( "type", "Point" )
                    .append( "coordinates", location1 );
            cl.createIndex(
                    Indexes.geoHaystack(
                            "geoHaystackField" + "." + "coordinates",
                            new Document( "category", 1 ) ),
                    new IndexOptions().bucketSize( 1.0 ) );
            for ( int i = 0; i < 50; i++ ) {
                cl.insertOne( new Document( "a", i ).append( "b", "str" + i )
                        .append( "location", location2 ) );
            }
            Document document = new Document( "geoSearch", collectionName )
                    .append( "near", location1 ).append( "maxDistance", 1000 )
                    .append( "limit", 10 )
                    .append( "search",
                            new Document( "a", new Document( "$gt", 0 ) ) )
                    .append( "readConcern", new Document( "level", "local" ) );
            if ( CommLib.compareVersion( mongoVerson, "4.4" ) >= 0 ) {
                document.append( "comment", "comment" );
            }
            database.runCommand( document );
            tearDown();
        }
    }

    /**
     * 构造queryAndWriteOp类型数据库命令 包含以下命令： delete find findAndModify getLastError
     * getMore insert resetError update
     */
    public void runQueryAndWriteCommands() {
        setUp();
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        MongoCollection< Document > cl = database
                .getCollection( collectionName );
        for ( int i = 0; i < 50; i++ ) {
            cl.insertOne( new Document( "a", i ).append( "b", "str" + i )
                    .append( "c", Arrays.asList( new Document( "d", i ),
                            new Document( "e", i ) ) ) );
        }
        cl.createIndex( new Document( "a", 1 ),
                new IndexOptions().name( "a_1" ) );
        cl.createIndex( new Document( "b", "text" ) );
        // delete
        Document deletes = new Document( "q",
                new Document( "a", new Document( "$gt", 50 ) ) )
                        .append( "limit", 1 );
        if ( CommLib.compareVersion( mongoVerson, "3.4" ) >= 0 ) {
            deletes.append( "collation", new Document( "locale", "en_US" ) );
        }
        if ( CommLib.compareVersion( mongoVerson, "4.4" ) >= 0 ) {
            deletes.append( "hint", "a_1" );
        }
        Document deleteDocument = new Document( "delete", collectionName )
                .append( "deletes", Arrays.asList( deletes ) )
                .append( "ordered", true )
                .append( "writeConcern",
                        new Document( "w", 0 ).append( "wtimeout", 1000 ) )
                .append( "maxTimeMS", 1000 );
        if ( CommLib.compareVersion( mongoVerson, "4.4" ) >= 0 ) {
            deleteDocument.append( "comment", "comment" );
        }
        database.runCommand( deleteDocument );

        // find
        // 覆盖查询操作符
        cl.find( new Document( "a", new Document( "$gt", 0 ) ) ).first();
        cl.find( new Document( "a", new Document( "$lt", 0 ) ) )
                .projection( Projections.excludeId() ).first();
        cl.find( new Document( "a", new Document( "$eq", 0 ) ) ).first();
        cl.find( new Document( "a", new Document( "$ne", 0 ) ) ).first();
        cl.find( new Document( "a", new Document( "$gte", 0 ) ) ).first();
        cl.find( new Document( "a", new Document( "$lte", 0 ) ) ).first();
        cl.find( new Document( "a",
                new Document( "$in", Arrays.asList( 0, 1 ) ) ) ).first();
        cl.find( new Document( "a",
                new Document( "$nin", Arrays.asList( 0, 1 ) ) ) ).first();
        cl.find( new Document( "a", new Document( "$exists", true ) ) ).first();
        cl.find( new Document( "a", new Document( "$type", 1 ) ) ).first();
        cl.find( new Document( "a",
                new Document( "$mod", Arrays.asList( 1, 1 ) ) ) ).first();
        cl.find( new Document( "a", new Document( "$regex", "str" ) ) ).first();
        cl.find( new Document( "$text", new Document( "$search", "str" ) ) )
                .first();
        cl.find( new Document( "$where", "this.a === 'str'" ) ).first();
        cl.find( new Document( "a", new Document( "$geoWithin", "str" ) ) )
                .first();
        cl.find( new Document( "a", new Document( "$geoIntersects", "str" ) ) )
                .first();
        cl.find( new Document( "a", new Document( "$near", "str" ) ) ).first();
        cl.find( new Document( "a", new Document( "$nearSphere", "str" ) ) )
                .first();
        cl.find( new Document( "a",
                new Document( "$all", Arrays.asList( 0, 1 ) ) ) ).first();
        cl.find( new Document( "a", new Document( "$elemMatch", "str" ) ) )
                .first();
        cl.find( new Document( "a", new Document( "$size", 1 ) ) ).first();
        cl.find( new Document( "a", new Document( "$bitsAllSet", 1 ) ) )
                .first();
        cl.find( new Document( "a", new Document( "$bitsAnySet", 1 ) ) )
                .first();
        cl.find( new Document( "a", new Document( "$bitsAllClear", 1 ) ) )
                .first();
        cl.find( new Document( "a", new Document( "$bitsAnyClear", 1 ) ) )
                .first();
        Document findDocument = new Document( "find", collectionName )
                .append( "filter",
                        new Document( "b",
                                new Document( "$bitsAllClear", 1 ) ) )
                .append( "projection", new Document( "a", 1 ).append( "b", 1 ) )
                .append( "sort", new Document( "a", 1 ) ).append( "skip", 0 )
                .append( "limit", 10 ).append( "batchSize", 10 )
                .append( "singleBatch", true )
                .append( "min", new Document( "a", 0 ) )
                .append( "max", new Document( "a", 100 ) )
                .append( "hint", "a_1" ).append( "comment", "comment" )
                .append( "returnKey", true ).append( "showRecordId", true )
                .append( "tailable", false ).append( "oplogReplay", true )
                .append( "noCursorTimeout", true ).append( "awaitData", false )
                .append( "allowPartialResults", true )
                .append( "readConcern", new Document( "level", "local" ) )
                .append( "maxTimeMS", 1000 );
        if ( CommLib.compareVersion( mongoVerson, "3.4" ) >= 0 ) {
            findDocument.append( "collation",
                    new Document( "locale", "en_US" ) );
        }
        if ( CommLib.compareVersion( mongoVerson, "4.4" ) >= 0 ) {
            findDocument.append( "allowDiskUse", true );
        }
        database.runCommand( findDocument );

        // findAndModify
        Document findAndModifyDocument = new Document( "findAndModify",
                collectionName )
                        .append( "query",
                                new Document( "where", "this.a > 50" ) )
                        .append( "sort", new Document( "a", 1 ) )
                        .append( "remove", false )
                        .append( "update",
                                new Document( "$set",
                                        new Document( "b", "str" ) ) )
                        .append( "new", true )
                        .append( "fields", new Document( "a", 1 ) )
                        .append( "upsert", true )
                        .append( "bypassDocumentValidation", true )
                        .append( "writeConcern", new Document( "w", 0 )
                                .append( "wtimeout", 1000 ) )
                        .append( "maxTimeMS", 1000 );
        if ( CommLib.compareVersion( mongoVerson, "3.4" ) >= 0 ) {
            findAndModifyDocument.append( "collation",
                    new Document( "locale", "en_US" ) );
        }
        if ( CommLib.compareVersion( mongoVerson, "4.4" ) >= 0 ) {
            findAndModifyDocument.append( "hint", "a_1" );
            findAndModifyDocument.append( "comment", "comment" );
        }
        database.runCommand( findAndModifyDocument );

        // getLastError
        Document getLastErrorDocument = new Document( "getLastError", 1 )
                .append( "w", 0 ).append( "wtimeout", 1000 )
                .append( "j", false );
        if ( CommLib.compareVersion( mongoVerson, "4.4" ) >= 0 ) {
            getLastErrorDocument.append( "comment", "comment" );
        }
        database.runCommand( getLastErrorDocument );

        // getMore
        MongoCursor< Document > comment = cl.find().batchSize( 10 )
                .maxTime( 1, TimeUnit.SECONDS ).comment( "comment" ).iterator();
        while ( comment.hasNext() ) {
            Document next = comment.next();
        }

        // insert
        Document insertDocument = new Document( "insert", collectionName )
                .append( "documents",
                        Arrays.asList( new Document( "a", 1 ),
                                new Document( "b", 2 ) ) )
                .append( "ordered", true )
                .append( "writeConcern",
                        new Document( "w", 0 ).append( "wtimeout", 1000 ) )
                .append( "bypassDocumentValidation", true )
                .append( "maxTimeMS", 1000 );
        if ( CommLib.compareVersion( mongoVerson, "4.4" ) >= 0 ) {
            insertDocument.append( "comment", "comment" );
        }
        database.runCommand( insertDocument );

        // resetError
        database.runCommand( new Document( "resetError", 1 ) );

        // update
        Document updateDocument = new Document( "update", collectionName )
                .append( "updates",
                        Arrays.asList(
                                new Document( "q", new Document( "a", 1 ) )
                                        .append( "u",
                                                new Document( "$set",
                                                        new Document( "b",
                                                                1 ) ) )
                                        .append( "upsert", true )
                                        .append( "multi", true ) ) )
                .append( "ordered", true )
                .append( "writeConcern",
                        new Document( "w", 0 ).append( "wtimeout", 1000 ) )
                .append( "bypassDocumentValidation", true )
                .append( "maxTimeMS", 1000 );
        tearDown();
    }

    /**
     * 构造queryPlanCache类型数据库命令 包含以下命令： planCacheClear planCacheClearFilters
     * planCacheListFilters // 该命令需要在planCacheFilter命令之后执行 planCacheSetFilter
     */
    public void runQueryPlanCacheCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造authentication类型数据库命令 包含以下命令： authenticate getnonce logout
     */
    public void runAuthenticationCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造userManagement类型数据库命令 包含以下命令： createUser dropAllUsersFromDatabase
     * dropUser grantRolesToUser revokeRolesFromUser updateUser usersInfo
     */
    public void runUserManagementCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造roleManagement类型数据库命令 包含以下命令： createRole dropAllRolesFromDatabase
     * dropRole grantPrivilegesToRole grantRolesToRole invalidateUserCache
     * revokePrivilegesFromRole revokeRolesFromRole rolesInfo updateRole
     */
    public void runRoleManagementCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造replication类型数据库命令 包含以下命令： appendOplogNote applyOps hello isMaster
     * replSetAbortPrimaryCatchUp replSetFreeze replSetGetConfig
     * replSetGetStatus replSetInitiate replSetMaintenance replSetReconfig
     * replSetResizeOplog replSetStepDown replSetSyncFrom
     */
    public void runReplicationCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造sharding类型数据库命令 包含以下命令： addShard addShardToZone
     * balancerCollectionStatus balancerStart balancerStatus balancerStop
     * checkShardingIndex clearJumboFlag cleanupOrphaned enableSharding
     * flushRouterConfig getShardMap getShardVersion isdbgrid listShards
     * medianKey moveChunk movePrimary mergeChunks refineCollectionShardKey
     * removeShard removeShardFromZone setAllowMigrate setShardVersion
     * shardCollection shardingState split splitVector unsetSharding
     * updateZoneKeyRange
     */
    public void runShardingCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造session类型数据库命令 包含以下命令： abortTransaction commitTransaction endSessions
     * killAllSessions killAllSessionsByPattern killSessions refreshSessions
     * startSession
     */
    public void runSessionCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造administration类型数据库命令 包含以下命令： cloneCollectionAsCapped collMod compact
     * convertToCapped create createIndexes currentOp drop dropDatabase
     * dropConnections dropIndexes filemd5 fsync fsyncUnlock getDefaultRWConcern
     * getParameter killCursors killOp listCollections listDatabases listIndexes
     * logRotate reIndex renameCollection setFeatureCompatibilityVersion
     * setIndexCommitQuorum setParameter setDefaultRWConcern shutdown
     */
    public void runAdministrationCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造diagnostic类型数据库命令 包含以下命令： availableQueryOptions buildInfo collStats
     * connPoolStats connectionStatus dataSize dbHash dbStats driverOIDTest
     * explain features getCmdLineOpts getLog hostInfo isSelf listCommands
     * lockInfo netstat ping profile serverStatus shardConnPoolStats top
     * validate whatsmyuri
     */
    public void runDiagnosticCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造freeMonitoring类型数据库命令 包含以下命令： getFreeMonitoringStatus setFreeMonitoring
     */
    public void runFreeMonitoringCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造systemEventsAuditing类型数据库命令 包含以下命令： logApplicationMessage
     */
    public void runSystemEventsAuditingCommands() {
        setUp();

        tearDown();
    }

    /**
     * 构造所有类型数据库命令
     */
    public void runAllCommands() {
        runAggregationCommands();
        runGeoSpatialCommands();
        runQueryAndWriteCommands();
        runQueryPlanCacheCommands();
        runAuthenticationCommands();
        runUserManagementCommands();
        runRoleManagementCommands();
        runReplicationCommands();
        runShardingCommands();
        runSessionCommands();
        runAdministrationCommands();
        runDiagnosticCommands();
        runFreeMonitoringCommands();
        runSystemEventsAuditingCommands();
    }

    private void tearDown() {
        mongoClient.getDatabase( databaseName ).drop();
    }

    public static void main( String[] args ) {
        MongoClient client = MongoClients.create( M2STestBase.mongodbUri );
        DataBaseCmd cmd = new DataBaseCmd( client );
        cmd.runQueryAndWriteCommands();
    }
}
