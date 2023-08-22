package com.mongodb.m2s.collector;

import com.alibaba.fastjson.JSONArray;
import org.bson.*;
import org.bson.types.BSONTimestamp;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoDatabase;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

import java.util.ArrayList;
import java.util.Date;

/**
 * @Descreption seqDB-32662:工具收集collection数据取样相关信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Collector_32662 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32662";
    private String collectionName = "col_32662";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        String[] allTypes = { "32-bit integer", "64-bit integer", "symbol",
                "code with scope", "array", "javascript", "embedded document",
                "regex", "dbPointer", "max key", "objectID", "null", "min key",
                "timestamp", "double", "string", "undefined", "UTC datetime",
                "binary", "boolean" };
        MongoDatabase mongoDatabase = mongoClient.getDatabase( databaseName );

        mongoDatabase.createCollection( collectionName );
        for ( int i = 0; i < 50; i++ ) {
            ArrayList< BsonInt32 > list = new ArrayList<>();
            list.add( new BsonInt32( i ) );
            Document document = new Document( "_id", new ObjectId() )
                    .append( "string", "str" + i )
                    .append( "int32", new BsonInt32( i ) )
                    .append( "int64", new BsonInt64( i * 1000 ) )
                    .append( "double", new BsonDouble( i ) )
                    .append( "date", new BsonDateTime( new Date().getTime() ) )
                    .append( "timestamp", new BSONTimestamp( 1, 1 ) )
                    .append( "binary", new BsonBinary( "test".getBytes() ) )
                    .append( "boolean", true ).append( "null", null )
                    .append( "array", list )
                    .append( "object",
                            new BsonDocument( "key", new BsonInt32( 1 ) ) )
                    .append( "regex", new BsonRegularExpression( ".*" ) )
                    .append( "code", new BsonJavaScript( "function(){}" ) )
                    .append( "symbol", new BsonSymbol( "symbol" ) )
                    .append( "dbPointer",
                            new BsonDbPointer( "db", new ObjectId() ) )
                    .append( "minKey", new BsonMinKey() )
                    .append( "maxKey", new BsonMaxKey() )
                    .append( "undefined", new BsonUndefined() )
                    .append( "codeWithScope",
                            new BsonJavaScriptWithScope( "function(){}",
                                    new BsonDocument().append( "key",
                                            new BsonInt32( i ) ) ) );
            mongoDatabase.getCollection( collectionName ).insertOne( document );
        }
        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );
        // 校验集合信息
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections1 = ssh.getStdout().split( "\n" );
        boolean containColl1 = false;
        for ( String str : collections1 ) {
            JSONObject collection = JSONObject.parseObject( str );
            if ( collection.getString( "database" ).equals( databaseName ) ) {
                if ( collection.getString( "name" ).equals( collectionName ) ) {
                    containColl1 = true;
                    // 文档数量为50
                    Assert.assertEquals( collection.getIntValue( "count" ),
                            50 );
                    // 文档抽样信息
                    JSONObject sample = JSONObject
                            .parseObject( collection.getString( "sample" ) );
                    // 包含数组元素
                    Assert.assertTrue( sample.getBoolean( "arrayField" ) );
                    // 最大嵌套层数为2
                    Assert.assertEquals( sample.getIntValue( "maxObjDepth" ),
                            2 );
                    // 最小嵌套层数为2
                    Assert.assertEquals( sample.getIntValue( "minObjDepth" ),
                            2 );
                    // 抽样数量为文档数量
                    Assert.assertEquals( sample.getIntValue( "n" ), 50 );
                    // 包含的数据类型
                    JSONArray fieldTypes = JSONObject
                            .parseArray( sample.getString( "fieldTypes" ) );
                    for ( int j = 0; j < allTypes.length; j++ ) {
                        Assert.assertTrue(
                                fieldTypes.contains( allTypes[ j ] ) );
                    }
                    break;
                }
            }
        }
        Assert.assertTrue( containColl1 );

        // 插入单条嵌套层数大的文档
        BsonDocument bsonDocument = new BsonDocument().append( "key1",
                new BsonDocument().append( "key2",
                        new BsonDocument().append( "key3", new BsonDocument()
                                .append( "key4", new BsonInt32( 1 ) ) ) ) );
        mongoDatabase.getCollection( collectionName )
                .insertOne( new Document( "object", bsonDocument ) );
        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );
        // 校验集合信息,最大嵌套层数发生改变，取样数发生改变
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections2 = ssh.getStdout().split( "\n" );
        for ( String str : collections2 ) {
            JSONObject collection = JSONObject.parseObject( str );
            if ( collection.getString( "database" ).equals( databaseName ) ) {
                if ( collection.getString( "name" ).equals( collectionName ) ) {
                    // 文档数量为51
                    Assert.assertEquals( collection.getIntValue( "count" ),
                            51 );
                    // 文档抽样信息
                    JSONObject sample = JSONObject
                            .parseObject( collection.getString( "sample" ) );
                    // 包含数组元素
                    Assert.assertTrue( sample.getBoolean( "arrayField" ) );
                    // 最大嵌套层数为5
                    Assert.assertEquals( sample.getIntValue( "maxObjDepth" ),
                            5 );
                    // 最小嵌套层数为2
                    Assert.assertEquals( sample.getIntValue( "minObjDepth" ),
                            2 );
                    // 抽样数量为文档数量
                    Assert.assertEquals( sample.getIntValue( "n" ), 51 );
                    // 包含的数据类型
                    JSONArray fieldTypes = JSONObject
                            .parseArray( sample.getString( "fieldTypes" ) );
                    for ( int j = 0; j < allTypes.length; j++ ) {
                        Assert.assertTrue(
                                fieldTypes.contains( allTypes[ j ] ) );
                    }
                    break;
                }
            }
        }

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null ) {
            mongoClient.close();
        }
        if ( ssh != null ) {
            ssh.disconnect();
        }
    }
}
