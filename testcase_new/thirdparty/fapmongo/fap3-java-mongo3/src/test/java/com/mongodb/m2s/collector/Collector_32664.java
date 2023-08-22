package com.mongodb.m2s.collector;

import com.mongodb.client.model.Collation;
import com.mongodb.client.model.CollationCaseFirst;
import org.bson.Document;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.MongoClient;
import com.mongodb.client.MongoClients;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.Indexes;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;

/**
 * @Descreption seqDB-32664:工具收集index全文索引信息
 * @Author chenzejia
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */
public class Collector_32664 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32664";
    private String collectionName = "col_32664";
    private String indexName = "index_32664";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, collectorOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {
        // 创建集合并插入数据
        MongoDatabase mongoDatabase = mongoClient.getDatabase( databaseName );
        MongoCollection< Document > collection = mongoDatabase
                .getCollection( collectionName );
        for ( int i = 0; i < 50; i++ ) {
            Document document = new Document( "name", "test" + i )
                    .append( "age", i );
            collection.insertOne( document );
        }

        // 创建全文索引
        collection.createIndex( Indexes.text( "name" ),
                new IndexOptions().name( indexName ) );

        // 收集集合信息
        CommLib.collectCollection( ssh, collectSample );

        boolean containIndex = false;
        // 校验集合信息
        ssh.exec( "cat " + collectorOutputPath + "collection.json" );
        String[] collections1 = ssh.getStdout().split( "\n" );
        for ( String str : collections1 ) {
            JSONObject collectionJson = JSONObject.parseObject( str );
            if ( collectionJson.getString( "database" )
                    .equals( databaseName ) ) {
                if ( collectionJson.getString( "name" )
                        .equals( collectionName ) ) {
                    JSONArray indexes = collectionJson
                            .getJSONArray( "indexes" );
                    for ( int i = 0; i < indexes.size(); i++ ) {
                        JSONObject index = indexes.getJSONObject( i );
                        if ( index.getString( "name" ).equals( indexName ) ) {
                            containIndex = true;
                            String expectKey = new Document( "_ftsx", 1 )
                                    .append( "_fts", "text" ).toJson();
                            Assert.assertEquals( index.getJSONObject( "key" ),
                                    JSONObject.parseObject( expectKey ) );
                            break;
                        }
                    }
                }
                break;
            }
        }
        Assert.assertTrue( containIndex );
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
