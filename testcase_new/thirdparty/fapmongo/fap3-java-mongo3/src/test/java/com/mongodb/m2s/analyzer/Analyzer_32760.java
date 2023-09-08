package com.mongodb.m2s.analyzer;

import org.bson.*;
import org.bson.types.BSONTimestamp;
import org.bson.types.Decimal128;
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
 * @Descreption seqDB-32760:覆盖所有数据类型，分析收集的集合信息文件
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/17
 * @UpdateRemark
 * @Version
 */
public class Analyzer_32760 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32760";
    private String collectionName = "col_32760";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        CommLib.initDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        String[] allTypes = { "32-bit integer", "64-bit integer",
                "128-bit decimal", "symbol", "code with scope", "array",
                "javascript", "embedded document", "regex", "dbPointer",
                "max key", "objectID", "null", "min key", "timestamp", "double",
                "string", "undefined", "UTC datetime", "binary", "boolean" };
        // 3.4版本才开始支持128-bit decimal
        String version = CommLib.getMongoDBVersion( mongoClient );
        boolean support128decimal = CommLib.compareVersion( version,
                "3.4" ) >= 0;
        MongoDatabase database = mongoClient.getDatabase( databaseName );
        database.createCollection( collectionName );

        // 插入数据，包含所有数据类型
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
            if ( support128decimal ) {
                document.append( "int128",
                        new BsonDecimal128( new Decimal128( i * 1000 ) ) );
            }
            database.getCollection( collectionName ).insertOne( document );
        }

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        // 分析集合信息
        String collectionJsonPath = collectorOutputPath + "collection.json";
        CommLib.analyzeCollection( ssh, collectionJsonPath );

        // 分析结果校验
        ssh.exec( "cat " + analyzerOutputPath + "collection.json" );
        JSONArray collections = JSONObject.parseArray( ssh.getStdout() );
        for ( int i = 0; i < collections.size(); i++ ) {
            JSONObject collection = collections.getJSONObject( i );
            if ( collection.getString( "collection" )
                    .equals( databaseName + "." + collectionName ) ) {
                // 校验集合包含所有数据类型
                JSONArray fieldType = collection.getJSONArray( "fieldType" );
                for ( int j = 0; j < allTypes.length; j++ ) {
                    if ( CommLib.compareVersion( version, "3.4" ) < 0
                            && allTypes[ j ].equals( "128-bit decimal" ) )
                        continue;
                    Assert.assertTrue( fieldType.contains( allTypes[ j ] ),
                            "集合中不包含" + allTypes[ j ] );
                }
                // 校验集合文档数为100
                Assert.assertEquals( collection.getIntValue( "count" ), 100 );
                // 检验集合最大嵌套层数为5
                Assert.assertEquals( collection.getIntValue( "maxObjectDepth" ),
                        5 );
                break;
            }
        }

    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, collectorOutputPath );
        CommLib.rmDir( ssh, analyzerOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

}
