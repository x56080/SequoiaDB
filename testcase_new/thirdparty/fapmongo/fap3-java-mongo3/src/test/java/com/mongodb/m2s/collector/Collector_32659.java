package com.mongodb.m2s.collector;

import org.bson.*;
import org.bson.types.BSONTimestamp;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONArray;
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
 * @Descreption seqDB-32659:工具收集collection集合信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/21
 * @UpdateRemark
 * @Version
 */
public class Collector_32659 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32659";
    private String collectionName1 = "col_32659_1";
    private String collectionName2 = "col_32659_2";

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

        mongoDatabase.createCollection( collectionName1 );
        for ( int i = 0; i < 100; i++ ) {
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
                    .append( "object", new BsonDocument().append( "key1",
                            new BsonDocument().append( "key2",
                                    new BsonDocument().append( "key3",
                                            new BsonDocument().append( "key4",
                                                    new BsonInt32( i ) ) ) ) ) )
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
            mongoDatabase.getCollection( collectionName1 ).insertOne( document );
        }
        CommLib.collectCluster( ssh );
        CommLib.collectCollection( ssh, collectSample );
        // 校验集群信息,目标数据库集合数量为1
        ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
        JSONObject cluster1 = JSONObject.parseObject( ssh.getStdout() );
        JSONArray databases1 = JSONObject
                .parseArray( cluster1.getString( "databases" ) );
        for ( int i = 0; i < databases1.size(); i++ ) {
            JSONObject database = databases1.getJSONObject( i );
            if ( database.getString( "name" ).equals( databaseName ) ) {
                Assert.assertEquals( database.getIntValue( "collectionNum" ),
                        1 );
                break;
            }
        }
        // 校验集合信息
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections1 = ssh.getStdout().split( "\n" );
        boolean containColl1 = false;
        for ( String i : collections1 ) {
            JSONObject collection = JSONObject.parseObject( i );
            if ( collection.getString( "database" ).equals( databaseName ) ) {
                if ( collection.getString( "name" ).equals( collectionName1 ) ) {
                    containColl1 = true;
                    // 文档数量为100
                    Assert.assertEquals( collection.getIntValue( "count" ),
                            100 );
                    // 文档抽样信息
                    JSONObject sample = JSONObject
                            .parseObject( collection.getString( "sample" ) );
                    // 包含数组元素
                    Assert.assertTrue( sample.getBoolean( "arrayField" ) );
                    // 最大嵌套层数为5
                    Assert.assertEquals( sample.getIntValue( "maxObjDepth" ),
                            5 );
                    // 最小嵌套层数为5
                    Assert.assertEquals( sample.getIntValue( "minObjDepth" ),
                            5 );
                    // 抽样数量为默认值100
                    Assert.assertEquals( sample.getIntValue( "n" ), 100 );
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

        // 继续创建集合并向之前的集合继续插入数据
        mongoDatabase.createCollection( collectionName2 );
        for ( int i = 0; i < 50; i++ ) {
            mongoDatabase.getCollection( collectionName2 )
                    .insertOne( new Document( "name", "test" + i ) );
            mongoDatabase.getCollection( collectionName1 )
                    .insertOne( new Document( "string", "test" + i )
                            .append( "int32", new BsonInt32( i ) ) );
        }

        // 校验集群信息,目标数据库集合数量为2
        CommLib.collectCluster( ssh );
        CommLib.collectCollection( ssh, collectSample );
        ssh.exec( "cat " + collectorOutputPath + "cluster.json" );
        JSONObject cluster2 = JSONObject.parseObject( ssh.getStdout() );
        JSONArray databases2 = JSONObject
                .parseArray( cluster2.getString( "databases" ) );
        for ( int i = 0; i < databases2.size(); i++ ) {
            JSONObject database = databases2.getJSONObject( i );
            if ( database.getString( "name" ).equals( databaseName ) ) {
                Assert.assertEquals( database.getIntValue( "collectionNum" ),
                        2 );
                break;
            }
        }
        // 校验集合信息
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections2 = ssh.getStdout().split( "\n" );
        boolean containColl2 = false;
        for ( String i : collections2 ) {
            JSONObject collection = JSONObject.parseObject( i );
            if ( collection.getString( "database" ).equals( databaseName ) ) {
                if ( collection.getString( "name" ).equals( collection ) ) {
                    // 文档数量为100
                    Assert.assertEquals( collection.getIntValue( "count" ),
                            150 );
                    // 文档抽样信息
                    JSONObject sample = JSONObject
                            .parseObject( collection.getString( "sample" ) );
                    // 最大嵌套层数为5
                    Assert.assertEquals( sample.getIntValue( "maxObjDepth" ),
                            5 );
                    // 最小嵌套层数为5
                    Assert.assertEquals( sample.getIntValue( "minObjDepth" ),
                            1 );
                }
                if ( collection.getString( "name" ).equals( collectionName2 ) ) {
                    containColl2 = true;
                    // 文档数量为50
                    Assert.assertEquals( collection.getIntValue( "count" ),
                            50 );
                    // 文档抽样信息
                    JSONObject sample = JSONObject
                            .parseObject( collection.getString( "sample" ) );
                    // 不包含数组元素
                    Assert.assertFalse( sample.getBoolean( "arrayField" ) );
                    // 最大嵌套层数为5
                    Assert.assertEquals( sample.getIntValue( "maxObjDepth" ),
                            1 );
                    // 最小嵌套层数为5
                    Assert.assertEquals( sample.getIntValue( "minObjDepth" ),
                            1 );
                    // 抽样数量为文档数量
                    Assert.assertEquals( sample.getIntValue( "n" ), 50 );
                    // 包含的数据类型
                    JSONArray fieldTypes = JSONObject
                            .parseArray( sample.getString( "fieldTypes" ) );
                    Assert.assertTrue( fieldTypes.contains( "string" ) );
                    Assert.assertTrue( fieldTypes.contains( "objectID" ) );
                }
            }
        }
        Assert.assertTrue( containColl2 );
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
