package com.mongodb.m2s.testcommon;

import com.mongodb.client.*;
import com.mongodb.client.model.*;
import org.bson.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.UUID;
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
    private String mongoVersion = null;

    public DataBaseCmd( MongoClient mongoClients ) {
        this.mongoClient = mongoClients;
        this.mongoVersion = CommLib.getMongoDBVersion( mongoClient );
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
        if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
            document.append( "collation", new Document( "locale", "en_US" ) );
        }
        adminClient.runCommand( document );

        // distinct
        document = new Document( "distinct", collectionName )
                .append( "key", "a" )
                .append( "query",
                        new Document( "a", new Document( "$gt", 0 ) ) )
                .append( "readConcern", new Document( "level", "local" ) );
        if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
            document.append( "collation", new Document( "locale", "en_US" ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            document.append( "comment", "comment" );
        }
        adminClient.runCommand( document );

        // mapReduce
        String mapFunction = "function() { emit(this.a, this.d); }";
        String reduceFunction = "function(key, values) { return Array.sum(values); }";

        Document mapReduceCommand = new Document( "mapreduce", collectionName )
                .append( "map", mapFunction ).append( "reduce", reduceFunction )
                .append( "query",
                        new Document( "a", new Document( "$exists", true ) ) )
                .append( "out", new Document( "merge", "outputCollection" ) )
                .append( "limit", 10 )
                .append( "finalize", "function(key, value) { return value; }" )
                .append( "scope", new Document( "scope", 1 ) )
                .append( "jsMode", true ).append( "verbose", true )
                .append( "bypassDocumentValidation", true )
                .append( "maxTimeMS", 1000 )
                .append( "readConcern", new Document( "level", "local" ) );
        database.runCommand( mapReduceCommand );
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
            if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
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
                    .append( "c",
                            Arrays.asList( new Document( "d", i ),
                                    new Document( "e", i ) ) )
                    .append( "f", i )
                    .append( "g", Arrays.asList( i, i * 2 ) ) );
        }
        Document location1 = new Document( "type", "Point" )
                .append( "coordinates", Arrays.asList( 10.0, 20.0 ) );
        Document document1 = new Document( "name", "Location 1" )
                .append( "location", location1 );
        cl.insertOne( document1 );

        Document location2 = new Document( "type", "Point" )
                .append( "coordinates", Arrays.asList( 15.0, 25.0 ) );
        Document document2 = new Document( "name", "Location 2" )
                .append( "location", location2 );
        cl.insertOne( document2 );

        // 创建索引
        cl.createIndex( new Document( "a", 1 ),
                new IndexOptions().name( "a_1" ) );
        cl.createIndex( new Document( "b", "text" ) );
        cl.createIndex( new Document( "location", "2dsphere" ) );
        // delete
        Document deletes = new Document( "q",
                new Document( "a", new Document( "$gt", 50 ) ) )
                        .append( "limit", 1 );
        if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
            deletes.append( "collation", new Document( "locale", "en_US" ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            deletes.append( "hint", "a_1" );
        }
        Document deleteDocument = new Document( "delete", collectionName )
                .append( "deletes", Arrays.asList( deletes ) )
                .append( "ordered", true )
                .append( "writeConcern",
                        new Document( "w", 0 ).append( "wtimeout", 1000 ) )
                .append( "maxTimeMS", 1000 );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            deleteDocument.append( "comment", "comment" );
        }
        database.runCommand( deleteDocument );

        // find
        // 覆盖查询操作符
        // $gt
        cl.find( new Document( "a", new Document( "$gt", 0 ) ) ).first();
        // $lt
        cl.find( new Document( "a", new Document( "$lt", 0 ) ) )
                .projection( Projections.excludeId() ).first();
        // $eq
        cl.find( new Document( "a", new Document( "$eq", 0 ) ) ).first();
        // $ne
        cl.find( new Document( "a", new Document( "$ne", 0 ) ) ).first();
        // $gte
        cl.find( new Document( "a", new Document( "$gte", 0 ) ) ).first();
        // $lte
        cl.find( new Document( "a", new Document( "$lte", 0 ) ) ).first();
        // $in
        cl.find( new Document( "a",
                new Document( "$in", Arrays.asList( 0, 1 ) ) ) ).first();
        // $nin
        cl.find( new Document( "a",
                new Document( "$nin", Arrays.asList( 0, 1 ) ) ) ).first();
        // $not
        cl.find( new Document( "a",
                new Document( "$not", new Document( "$gt", 0 ) ) ) ).first();
        // $nor
        cl.find( new Document( "$nor", Arrays.asList( new Document( "a", 0 ),
                new Document( "b", 1 ) ) ) ).first();
        // $and
        cl.find( new Document( "$and", Arrays.asList( new Document( "a", 0 ),
                new Document( "b", 1 ) ) ) ).first();
        // $or
        cl.find( new Document( "$or", Arrays.asList( new Document( "a", 0 ),
                new Document( "b", 1 ) ) ) ).first();
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            // $expr
            cl.find( new Document( "$expr",
                    new Document( "$gt", Arrays.asList( "$a", "$b" ) ) ) )
                    .first();
            // $jsonSchema
            cl.find( new Document( "$jsonSchema",
                    new Document( "required", Arrays.asList( "a" ) ) ) )
                    .first();
        }
        // $exists
        cl.find( new Document( "a", new Document( "$exists", true ) ) ).first();
        // $type
        cl.find( new Document( "a", new Document( "$type", 1 ) ) ).first();
        // $mod
        cl.find( new Document( "a",
                new Document( "$mod", Arrays.asList( 1, 1 ) ) ) ).first();
        // $regex
        cl.find( new Document( "a", new Document( "$regex", "str" ) ) ).first();
        // $text
        cl.find( new Document( "$text", new Document( "$search", "str" ) ) )
                .first();
        // $where
        cl.find( new Document( "$where", "this.a === 'str'" ) ).first();
        Document geoJsonPolygon = new Document( "type", "Polygon" ).append(
                "coordinates",
                Arrays.asList( Arrays.asList( Arrays.asList( 5.0, 15.0 ),
                        Arrays.asList( 5.0, 30.0 ), Arrays.asList( 20.0, 30.0 ),
                        Arrays.asList( 20.0, 15.0 ),
                        Arrays.asList( 5.0, 15.0 ) ) ) );
        // $geoWithin
        cl.find( new Document( "location",
                new Document( "$geoWithin",
                        new Document( "$geometry", geoJsonPolygon ) ) ) )
                .first();
        // 创建地理对象
        Document geoJsonPoint = new Document( "type", "Point" )
                .append( "coordinates", Arrays.asList( 10.0, 20.0 ) );
        // $geoIntersects
        cl.find( new Document( "location", new Document( "$geoIntersects",
                new Document( "$geometry", geoJsonPoint ) ) ) ).first();
        // $near
        cl.find( new Document( "location", new Document( "$near",
                new Document( "$geometry", geoJsonPoint ) ) ) ).first();
        // $nearSphere
        cl.find( new Document( "location", new Document( "$nearSphere",
                new Document( "$geometry", geoJsonPoint ) ) ) ).first();
        // $all
        cl.find( new Document( "a",
                new Document( "$all", Arrays.asList( 0, 1 ) ) ) ).first();
        // $elemMatch
        cl.find( new Document( "c",
                new Document( "$elemMatch",
                        new Document( "d", new Document( "$gt", 25 ) ) ) ) )
                .first();
        // $size
        cl.find( new Document( "a", new Document( "$size", 1 ) ) ).first();
        // $bitsAllSet
        cl.find( new Document( "a", new Document( "$bitsAllSet", 1 ) ) )
                .first();
        // $bitsAnySet
        cl.find( new Document( "a", new Document( "$bitsAnySet", 1 ) ) )
                .first();
        // $bitsAllClear
        cl.find( new Document( "a", new Document( "$bitsAllClear", 1 ) ) )
                .first();
        // $bitsAnyClear
        cl.find( new Document( "a", new Document( "$bitsAnyClear", 1 ) ) )
                .first();
        // $comment
        cl.find( new Document( new Document( "$comment", "comment" ) ) )
                .first();
        // $rand
        if ( CommLib.compareVersion( mongoVersion, "4.4.2" ) >= 0 ) {
            cl.find( new Document( "$expr",
                    new Document( "$gt", Arrays.asList( 1,
                            new Document( "$rand", new Document() ) ) ) ) )
                    .first();
        }
        // $slice
        Document slice = new Document( "c",
                new Document( "$slice", Arrays.asList( 1, 1 ) ) );
        cl.find().projection( slice ).first();
        // $meta
        cl.find( new Document( "$text",
                new Document( "$search", "searchText" ) ) )
                .projection( new Document( "a", 1 ).append( "textScore",
                        new Document( "$meta", "textScore" ) ) )
                .first();
        // $
        cl.find( new Document( "a.$", 1 ) ).first();

        // 覆盖数据库命令参数
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
                .append( "tailable", false ).append( "oplogReplay", false )
                .append( "noCursorTimeout", true ).append( "awaitData", false )
                .append( "allowPartialResults", true )
                .append( "readConcern", new Document( "level", "local" ) )
                .append( "maxTimeMS", 1000 );
        if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
            findDocument.append( "collation",
                    new Document( "locale", "en_US" ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
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
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            findAndModifyDocument = new Document( "findAndModify",
                    collectionName ).append( "query", new Document( "a", 1 ) )
                            .append( "sort", new Document( "a", 1 ) )
                            .append( "remove", false )
                            .append( "update",
                                    new Document( "$set",
                                            new Document( "g.$[i]", 1 ) ) )
                            .append( "new", true )
                            .append( "fields", new Document( "a", 1 ) )
                            .append( "upsert", true )
                            .append( "bypassDocumentValidation", true )
                            .append( "writeConcern",
                                    new Document( "w", 0 ).append( "wtimeout",
                                            1000 ) )
                            .append( "maxTimeMS", 1000 ).append( "arrayFilters",
                                    Arrays.asList( new Document( "i", 1 ) ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
            findAndModifyDocument.append( "collation",
                    new Document( "locale", "en_US" ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            findAndModifyDocument.append( "hint", "a_1" );
            findAndModifyDocument.append( "comment", "comment" );
        }
        database.runCommand( findAndModifyDocument );

        // getLastError
        Document getLastErrorDocument = new Document( "getLastError", 1 )
                .append( "w", 0 ).append( "wtimeout", 1000 )
                .append( "j", false );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
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
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            insertDocument.append( "comment", "comment" );
        }
        database.runCommand( insertDocument );

        // resetError
        database.runCommand( new Document( "resetError", 1 ) );

        // update
        cl.insertOne( new Document( "a", 1 ).append( "b", "str" )
                .append( "date", new BsonDateTime( new Date().getTime() ) ) );
        // 覆盖所有update操作符
        // $currentDate
        cl.updateOne( new Document( "a", 1 ), new Document( "$currentDate",
                new Document( "date", new Document( "$type", "date" ) ) ) );
        // $inc
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$inc", new Document( "f", 1 ) ) );
        // $min
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$min", new Document( "f", 1 ) ) );
        // $max
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$max", new Document( "f", 1 ) ) );
        // $mul
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$mul", new Document( "f", 1 ) ) );
        // $rename
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$rename", new Document( "b", "bb" ) ) );
        // $set
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$set", new Document( "f", 1 ) ) );
        // $setOnInsert
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$setOnInsert", new Document( "f", 1 ) ) );
        // $unset
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$unset", new Document( "f", 1 ) ) );
        // $addToSet
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$addToSet", new Document( "g", 1 ) ) );
        // $pop
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$pop", new Document( "g", 1 ) ) );
        // $pullAll
        cl.updateOne( new Document( "a", 1 ), new Document( "$pullAll",
                new Document( "g", Arrays.asList( 1, 2, 3 ) ) ) );
        // $pull
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$pull", new Document( "g", 1 ) ) );
        // $push
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$push", new Document( "g", 1 ) ) );
        // $bit
        cl.updateOne( new Document( "a", 1 ), new Document( "$bit",
                new Document( "a", new Document( "and", 1 ) ) ) );
        // $each
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$push", new Document( "g",
                        new Document( "$each", Arrays.asList( 1, 2 ) ) ) ) );
        // $slice
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$push",
                        new Document( "g",
                                new Document( "$each", Arrays.asList( 1, 2 ) )
                                        .append( "$slice", 5 ) ) ) );
        // $sort
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$push",
                        new Document( "c", new Document( "$each",
                                Arrays.asList( new Document( "d", 1 )
                                        .append( "e", 1 ) ) ).append( "$sort",
                                                new Document( "d", 1 ) ) ) ) );
        // $position
        cl.updateOne( new Document( "a", 1 ),
                new Document( "$push",
                        new Document( "g",
                                new Document( "$each", Arrays.asList( 1, 2 ) )
                                        .append( "$position", 0 ) ) ) );
        // $
        cl.updateOne( new Document( "a", 1 ).append( "g", 1 ),
                new Document( "$set", new Document( "g.$", 1 ) ) );
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            // $[]
            cl.updateOne( new Document( "a", 1 ),
                    new Document( "$set", new Document( "g.$[]", 1 ) ) );
            // $[<identifier>]
            cl.updateOne( new Document( "a", 1 ).append( "g", 1 ),
                    new Document( "$set", new Document( "g.$[i]", 1 ) ),
                    new UpdateOptions().arrayFilters(
                            Arrays.asList( new Document( "i", 1 ) ) ) );
        }
        Document updates = new Document( "q", new Document( "a", 1 ) )
                .append( "u", new Document( "$set", new Document( "f", 1 ) ) )
                .append( "upsert", true ).append( "multi", true );
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            updates = new Document( "q", new Document( "a", 1 ) )
                    .append( "u",
                            new Document( "$set",
                                    new Document( "g.$[i]", 1 ) ) )
                    .append( "upsert", true ).append( "multi", true )
                    .append( "arrayFilters",
                            Arrays.asList( new Document( "i", 1 ) ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
            updates.append( "collation", new Document( "locale", "en_US" ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.2" ) >= 0 ) {
            updates.append( "hint", new Document( "a", 1 ) );
        }
        Document updateDocument = new Document( "update", collectionName )
                .append( "updates", Arrays.asList( updates ) )
                .append( "ordered", true )
                .append( "writeConcern",
                        new Document( "w", 0 ).append( "wtimeout", 1000 ) )
                .append( "bypassDocumentValidation", true )
                .append( "maxTimeMS", 1000 );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            updateDocument.append( "comment", "comment" );
        }
        database.runCommand( updateDocument );
        tearDown();
    }

    /**
     * 构造queryPlanCache类型数据库命令 包含以下命令： planCacheClear planCacheClearFilters
     * planCacheListFilters planCacheSetFilter
     */
    public void runQueryPlanCacheCommands() {
        setUp();
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        MongoCollection< Document > cl = database
                .getCollection( collectionName );
        for ( int i = 0; i < 10; i++ ) {
            cl.insertOne( new Document( "a", i ).append( "b", i ) );
        }
        cl.createIndex( new Document( "a", 1 ) );
        // planCacheClear
        Document planCacheClear = new Document( "planCacheClear",
                collectionName ).append( "query", new Document( "a", 1 ) )
                        .append( "projection", new Document( "a", 1 ) )
                        .append( "sort", new Document( "a", 1 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            planCacheClear.append( "comment", "comment" );
        }
        database.runCommand( planCacheClear );

        // planCacheClearFilters
        Document planCacheClearFilters = new Document( "planCacheClearFilters",
                collectionName ).append( "query", new Document( "a", 1 ) )
                        .append( "projection", new Document( "a", 1 ) )
                        .append( "sort", new Document( "a", 1 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            planCacheClearFilters.append( "comment", "comment" );
        }
        database.runCommand( planCacheClearFilters );

        // planCacheListFilters
        Document planCacheListFilters = new Document( "planCacheListFilters",
                collectionName );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            planCacheListFilters.append( "comment", "comment" );
        }
        database.runCommand( planCacheListFilters );

        // planCacheSetFilter
        Document planCacheSetFilter = new Document( "planCacheSetFilter",
                collectionName ).append( "query", new Document( "a", 1 ) )
                        .append( "projection", new Document( "a", 1 ) )
                        .append( "sort", new Document( "a", 1 ) )
                        .append( "indexes",
                                Arrays.asList( new Document( "a", 1 ) ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            planCacheSetFilter.append( "comment", "comment" );
        }
        database.runCommand( planCacheSetFilter );
        tearDown();
    }

    /**
     * 构造authentication类型数据库命令 包含以下命令： authenticate getnonce logout
     */
    public void runAuthenticationCommands() {
        setUp();
        MongoDatabase database = mongoClient.getDatabase( "admin" );
        // authenticate
        // database.runCommand( new Document( "authenticate", 1 ));
        // getnonce
        database.runCommand( new Document( "getnonce", 1 ) );
        // logout
        database.runCommand( new Document( "logout", 1 ) );
        tearDown();
    }

    /**
     * 构造userManagement类型数据库命令 包含以下命令： createUser dropAllUsersFromDatabase
     * dropUser grantRolesToUser revokeRolesFromUser updateUser usersInfo
     */
    public void runUserManagementCommands() {
        setUp();
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        // createUser
        Document createUser = new Document( "createUser", "user" )
                .append( "pwd", "pwd" )
                .append( "roles",
                        Arrays.asList( new Document( "role", "readWrite" )
                                .append( "db", databaseName ) ) )
                .append( "customData", new Document( "employeeId", 1 ) )
                .append( "writeConcern",
                        new Document( "w", 1 ).append( "wtimeout", 1000 ) )
                .append( "digestPassword", false );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            createUser.append( "comment", "comment" );
        }
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            createUser.append( "authenticationRestrictions", Arrays
                    .asList( new Document( "clientSource", Arrays.asList() )
                            .append( "serverAddress", Arrays.asList() ) ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.0" ) >= 0 ) {
            createUser.append( "mechanisms", Arrays.asList( "SCRAM-SHA-1" ) );
        }
        database.runCommand( createUser );
        // dropAllUsersFromDatabase
        Document dropAllUsersFromDatabase = new Document(
                "dropAllUsersFromDatabase", 1 ).append( "writeConcern",
                        new Document( "w", 1 ).append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            dropAllUsersFromDatabase.append( "comment", "comment" );
        }
        database.runCommand( dropAllUsersFromDatabase );
        // revokeRolesFromUser
        database.runCommand( createUser );
        Document revokeRolesFromUser = new Document( "revokeRolesFromUser",
                "user" ).append(
                        "roles",
                        Arrays.asList( new Document( "role", "readWrite" )
                                .append( "db", databaseName ) ) )
                        .append( "writeConcern", new Document( "w", 1 )
                                .append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            revokeRolesFromUser.append( "comment", "comment" );
        }
        database.runCommand( revokeRolesFromUser );
        // grantRolesToUser
        Document grantRolesToUser = new Document( "grantRolesToUser", "user" )
                .append( "roles",
                        Arrays.asList( new Document( "role", "readWrite" )
                                .append( "db", databaseName ) ) )
                .append( "writeConcern",
                        new Document( "w", 1 ).append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            grantRolesToUser.append( "comment", "comment" );
        }
        database.runCommand( grantRolesToUser );
        // updateUser
        Document updateUser = new Document( "updateUser", "user" )
                .append( "pwd", "pwd" )
                .append( "roles",
                        Arrays.asList( new Document( "role", "readWrite" )
                                .append( "db", databaseName ) ) )
                .append( "customData", new Document( "employeeId", 1 ) )
                .append( "writeConcern",
                        new Document( "w", 1 ).append( "wtimeout", 1000 ) )
                .append( "digestPassword", false );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            updateUser.append( "comment", "comment" );
        }
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            updateUser.append( "authenticationRestrictions", Arrays
                    .asList( new Document( "clientSource", Arrays.asList() )
                            .append( "serverAddress", Arrays.asList() ) ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.0" ) >= 0 ) {
            updateUser.append( "mechanisms", Arrays.asList( "SCRAM-SHA-1" ) );
        }
        database.runCommand( updateUser );
        // usersInfo
        Document usersInfo = new Document( "usersInfo", 1 )
                .append( "showCredentials", true )
                .append( "showPrivileges", false );
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            usersInfo.append( "showAuthenticationRestrictions", true );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.0" ) >= 0 ) {
            usersInfo.append( "filter", new Document( "user", "user" ) );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            usersInfo.append( "comment", "comment" );
        }
        database.runCommand( usersInfo );
        // dropUser
        Document dropUser = new Document( "dropUser", "user" ).append(
                "writeConcern",
                new Document( "w", 1 ).append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            dropUser.append( "comment", "comment" );
        }
        database.runCommand( dropUser );
        tearDown();
    }

    /**
     * 构造roleManagement类型数据库命令 包含以下命令： createRole dropAllRolesFromDatabase
     * dropRole grantPrivilegesToRole grantRolesToRole invalidateUserCache
     * revokePrivilegesFromRole revokeRolesFromRole rolesInfo updateRole
     */
    public void runRoleManagementCommands() {
        setUp();
        String roleName = "customRole";
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        database.createCollection( collectionName );
        // createRole
        Document createRole = new Document( "createRole", roleName )
                .append( "privileges",
                        Arrays.asList( new Document( "resource",
                                new Document( "db", databaseName ).append(
                                        "collection", collectionName ) ).append(
                                                "actions",
                                                Arrays.asList( "find" ) ) ) )
                .append( "roles", Arrays.asList() ).append( "writeConcern",
                        new Document( "w", 1 ).append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            createRole.append( "comment", "comment" );
        }
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            createRole.append( "authenticationRestrictions", Arrays
                    .asList( new Document( "clientSource", Arrays.asList() )
                            .append( "serverAddress", Arrays.asList() ) ) );
        }
        database.runCommand( createRole );
        // dropAllRolesFromDatabase
        Document dropAllRolesFromDatabase = new Document(
                "dropAllRolesFromDatabase", 1 ).append( "writeConcern",
                        new Document( "w", 1 ).append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            dropAllRolesFromDatabase.append( "comment", "comment" );
        }
        database.runCommand( dropAllRolesFromDatabase );
        database.runCommand( createRole );
        // dropRole
        Document dropRole = new Document( "dropRole", roleName ).append(
                "writeConcern",
                new Document( "w", 1 ).append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            dropRole.append( "comment", "comment" );
        }
        database.runCommand( dropRole );
        database.runCommand( createRole );
        // revokePrivilegesFromRole
        Document revokePrivilegesFromRole = new Document(
                "revokePrivilegesFromRole", roleName ).append(
                        "privileges",
                        Arrays.asList( new Document( "resource",
                                new Document( "db", databaseName ).append(
                                        "collection", collectionName ) ).append(
                                                "actions",
                                                Arrays.asList( "find" ) ) ) )
                        .append( "writeConcern", new Document( "w", 1 )
                                .append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            revokePrivilegesFromRole.append( "comment", "comment" );
        }
        database.runCommand( revokePrivilegesFromRole );
        // grantPrivilegesToRole
        Document grantPrivilegesToRole = new Document( "grantPrivilegesToRole",
                roleName ).append(
                        "privileges",
                        Arrays.asList( new Document( "resource",
                                new Document( "db", databaseName ).append(
                                        "collection", collectionName ) ).append(
                                                "actions",
                                                Arrays.asList( "find" ) ) ) )
                        .append( "writeConcern", new Document( "w", 1 )
                                .append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            grantPrivilegesToRole.append( "comment", "comment" );
        }
        database.runCommand( grantPrivilegesToRole );
        // grantRolesToRole
        Document grantRolesToRole = new Document( "grantRolesToRole", roleName )
                .append( "roles",
                        Arrays.asList( new Document( "role", "readWrite" )
                                .append( "db", databaseName ) ) )
                .append( "writeConcern",
                        new Document( "w", 1 ).append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            grantRolesToRole.append( "comment", "comment" );
        }
        database.runCommand( grantRolesToRole );
        // invalidateUserCache
        mongoClient.getDatabase( "admin" )
                .runCommand( new Document( "invalidateUserCache", 1 ) );
        // revokeRolesFromRole
        Document revokeRolesFromRole = new Document( "revokeRolesFromRole",
                roleName ).append(
                        "roles",
                        Arrays.asList( new Document( "role", "readWrite" )
                                .append( "db", databaseName ) ) )
                        .append( "writeConcern", new Document( "w", 1 )
                                .append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            revokeRolesFromRole.append( "comment", "comment" );
        }
        database.runCommand( revokeRolesFromRole );
        // rolesInfo
        Document rolesInfo = new Document( "rolesInfo",
                new Document( "role", roleName ).append( "db", databaseName ) )
                        .append( "showPrivileges", false )
                        .append( "showBuiltinRoles", true );
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            rolesInfo.append( "showAuthenticationRestrictions", false );
        }
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            rolesInfo.append( "comment", "comment" );
        }
        database.runCommand( rolesInfo );
        // updateRole
        Document updateRole = new Document( "updateRole", roleName )
                .append( "privileges",
                        Arrays.asList( new Document( "resource",
                                new Document( "db", databaseName ).append(
                                        "collection", collectionName ) ).append(
                                                "actions",
                                                Arrays.asList( "find" ) ) ) )
                .append( "roles", Arrays.asList() ).append( "writeConcern",
                        new Document( "w", 1 ).append( "wtimeout", 1000 ) );
        if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
            createRole.append( "comment", "comment" );
        }
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            createRole.append( "authenticationRestrictions", Arrays
                    .asList( new Document( "clientSource", Arrays.asList() )
                            .append( "serverAddress", Arrays.asList() ) ) );
        }
        database.runCommand( updateRole );
        database.runCommand( dropRole );
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
        MongoDatabase adminDatabase = mongoClient.getDatabase( "admin" );
        // applyOps(internal command)
        // hello
        if ( CommLib.compareVersion( mongoVersion, "3.6.21" ) >= 0 ) {
            adminDatabase.runCommand( new Document( "hello", 1 ) );
        }
        // isMaster
        adminDatabase.runCommand( new Document( "isMaster", 1 ) );
        if ( CommLib.isReplicaSet( mongoClient ) ) {
            // appendOplogNote
            adminDatabase.runCommand( new Document( "appendOplogNote", 1 )
                    .append( "data", new Document( "msg", "note" ) ) );
            try {
                // replSetAbortPrimaryCatchUp
                adminDatabase.runCommand(
                        new Document( "replSetAbortPrimaryCatchUp", 1 ) );
            } catch ( Exception e ) {
            }
            try {
                // replSetFreeze
                adminDatabase.runCommand( new Document( "replSetFreeze", 1 ) );
            } catch ( Exception e ) {
            }
            try {
                // replSetMaintenance
                adminDatabase.runCommand(
                        new Document( "replSetMaintenance", false ) );
            } catch ( Exception e ) {
            }
            // replSetGetConfig
            Document replSetGetConfig = new Document( "replSetGetConfig", 1 );
            if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
                replSetGetConfig.append( "commitmentStatus", true )
                        .append( "comment", "comment" );
            }
            adminDatabase.runCommand( replSetGetConfig );
            // replSetGetStatus
            adminDatabase.runCommand( new Document( "replSetGetStatus", 1 ) );
            // replSetResizeOplog
            if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
                adminDatabase
                        .runCommand( new Document( "replSetResizeOplog", 1 )
                                .append( "size", 1000.0 ) );
            }
            // replSetInitiate
            // replSetReconfig
            // replSetStepDown
            // replSetSyncFrom
        }
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
        MongoDatabase adminDatabase = mongoClient.getDatabase( "admin" );
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        database.createCollection( collectionName );
        if ( CommLib.isSharded( mongoClient ) ) {
            // enableSharding
            adminDatabase.runCommand(
                    new Document( "enableSharding", databaseName ) );
            // shardCollection
            adminDatabase.runCommand( new Document( "shardCollection",
                    databaseName + "." + collectionName ).append( "key",
                            new Document( "key", 1 ) ) );
            // addShard
            try {
                adminDatabase.runCommand( new Document( "addShard", "shard" )
                        .append( "maxSize", 0 ).append( "name", "shard1" ) );
            } catch ( Exception e ) {
            }
            // addShardToZone
            try {
                if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
                    adminDatabase.runCommand(
                            new Document( "addShardToZone", "shard" )
                                    .append( "zone", "zone" ) );
                }
            } catch ( Exception e ) {
            }
            // balancerCollectionStatus
            try {
                if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 )
                    adminDatabase.runCommand( new Document(
                            "balancerCollectionStatus", "db.collection" ) );
            } catch ( Exception e ) {
            }
            if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
                // balancerStatus
                adminDatabase.runCommand( new Document( "balancerStatus", 1 ) );
                // balancerStop
                adminDatabase.runCommand( new Document( "balancerStop", 1 )
                        .append( "maxTimeMS", 100 ) );
                // balancerStart
                adminDatabase.runCommand( new Document( "balancerStart", 1 )
                        .append( "maxTimeMS", 100 ) );
            }
            // checkShardingIndex(internal command)
            // clearJumboFlag
            if ( CommLib.compareVersion( mongoVersion, "4.0.15" ) >= 0
                    && CommLib.compareVersion( mongoVersion, "4.2.3" ) <= 0 ) {
                try {
                    adminDatabase.runCommand(
                            new Document( "clearJumboFlag", "db.collection" )
                                    .append( "find", new Document() ) );
                } catch ( Exception e ) {
                }
            }
            // flushRouterConfig
            adminDatabase.runCommand(
                    new Document( "flushRouterConfig", databaseName ) );
            // getShardMap
            adminDatabase.runCommand( new Document( "getShardMap", 1 ) );
            // getShardVersion
            adminDatabase.runCommand( new Document( "getShardVersion",
                    databaseName + "." + collectionName ) );
            // isdbgrid
            adminDatabase.runCommand( new Document( "isdbgrid", 1 ) );
            // listShards
            adminDatabase.runCommand( new Document( "listShards", 1 ) );
            // medianKey(internal command)
            // moveChunk
            try {
                adminDatabase.runCommand( new Document( "moveChunk",
                        databaseName + "." + collectionName )
                                .append( "find", new Document( "key", 1 ) )
                                .append( "to", "shard" ) );
            } catch ( Exception e ) {
            }
            // movePrimary
            try {
                adminDatabase
                        .runCommand( new Document( "movePrimary", databaseName )
                                .append( "to", "shard" ) );
            } catch ( Exception e ) {
            }
            // mergeChunks
            try {
                adminDatabase.runCommand( new Document( "mergeChunks",
                        databaseName + "." + collectionName ).append( "bounds",
                                Arrays.asList( new Document( "key", 1 ),
                                        new Document( "key", 2 ) ) ) );
            } catch ( Exception e ) {
            }
            // refineCollectionShardKey
            if ( CommLib.compareVersion( mongoVersion, "4.4" ) >= 0 ) {
                try {
                    adminDatabase.runCommand( new Document(
                            "refineCollectionShardKey",
                            databaseName + "." + collectionName ).append( "key",
                                    new Document( "key1", 1 ) ) );
                } catch ( Exception e ) {
                }
            }
            // removeShard
            try {
                adminDatabase
                        .runCommand( new Document( "removeShard", "shard" ) );
            } catch ( Exception e ) {
            }
            // removeShardFromZone
            try {
                if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
                    adminDatabase.runCommand(
                            new Document( "removeShardFromZone", "shard" )
                                    .append( "zone", "zone" ) );
                }
            } catch ( Exception e ) {
            }
            // setShardVersion(internal command)
            // setAllowMigrate
            if ( CommLib.compareVersion( mongoVersion, "4.4.11" ) >= 0 ) {
                try {
                    adminDatabase.runCommand( new Document( "setAllowMigrate",
                            databaseName + "." + collectionName )
                                    .append( "allowMigrations", true ) );
                } catch ( Exception e ) {
                }
            }
            // shardingState
            // split
            try {
                adminDatabase.runCommand( new Document( "split",
                        databaseName + "." + collectionName ).append( "middle",
                                new Document( "key", 1 ) ) );
            } catch ( Exception e ) {
            }
            // splitVector(internal command)
            // unsetSharding(internal command)
            // updateZoneKeyRange
            try {
                if ( CommLib.compareVersion( mongoVersion, "3.4" ) >= 0 ) {
                    adminDatabase
                            .runCommand( new Document( "updateZoneKeyRange",
                                    databaseName + "." + collectionName )
                                            .append( "min",
                                                    new Document( "key", 1 ) )
                                            .append( "max",
                                                    new Document( "key", 2 ) )
                                            .append( "zone", "zone" ) );
                }
            } catch ( Exception e ) {
            }
        }
        tearDown();
    }

    /**
     * 构造session类型数据库命令 包含以下命令： abortTransaction commitTransaction endSessions
     * killAllSessions killAllSessionsByPattern killSessions refreshSessions
     * startSession
     */
    public void runSessionCommands() {
        setUp();
        MongoDatabase adminDatabase = mongoClient.getDatabase( "admin" );
        if ( CommLib.compareVersion( mongoVersion, "3.6" ) >= 0 ) {
            // startSession
            Document document = adminDatabase
                    .runCommand( new Document( "startSession", 1 ) );
            Document id = ( Document ) document.get( "id" );
            UUID uuid = ( UUID ) id.get( "id" );
            // refreshSessions
            adminDatabase.runCommand( new Document( "refreshSessions",
                    Arrays.asList( new BsonDocument( "id",
                            new BsonBinary( uuid ) ) ) ) );
            if ( CommLib.compareVersion( mongoVersion, "4.0" ) >= 0 ) {
                try {
                    // abortTransaction
                    adminDatabase.runCommand(
                            new Document( "abortTransaction", 1 ) );
                } catch ( Exception e ) {
                }
                try {
                    // commitTransaction
                    adminDatabase.runCommand(
                            new Document( "commitTransaction", 1 ) );
                } catch ( Exception e ) {
                }
            }
            // killAllSessions
            try {
                adminDatabase.runCommand(
                        new Document( "killAllSessions", Arrays.asList() ) );
            } catch ( Exception e ) {
            }
            // killAllSessionsByPattern
            try {
                adminDatabase.runCommand( new Document(
                        "killAllSessionsByPattern", Arrays.asList() ) );
            } catch ( Exception e ) {
            }
            // killSessions
            try {
                adminDatabase.runCommand( new Document( "killSessions",
                        Arrays.asList( new BsonDocument( "id",
                                new BsonBinary( uuid ) ) ) ) );
            } catch ( Exception e ) {
            }
            // endSessions
            adminDatabase.runCommand( new Document( "endSessions",
                    Arrays.asList( new BsonDocument( "id",
                            new BsonBinary( uuid ) ) ) ) );
        }

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
        cmd.runAggregationCommands();
        client.close();
    }
}
