package com.mongodb.m2s.sniffer;

import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.mongodb.client.*;
import com.mongodb.client.model.CreateCollectionOptions;
import com.mongodb.client.model.IndexOptions;
import com.mongodb.client.model.InsertManyOptions;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.m2s.testcommon.Param;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * @Descreption seqDB-32610:执行查询时相关游标操作检测，查看消息分析报告
 *              seqDB-32611:执行创建集合、创建索引操作，检测消息分析报告
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */

public class Sniffer_32610_32611 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32610_32611";
    private String collectionName = "coll_32610_32611";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, snifferOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {

        CommLib.snifferStart( ssh );

        // create collection
        MongoClient snifferClient = CommLib.getSnifferClient();
        MongoDatabase database = snifferClient.getDatabase( databaseName );
        database.createCollection( collectionName );
        MongoCollection< Document > cl = database
                .getCollection( collectionName );

        // insert data
        ArrayList< Document > docs = new ArrayList<>();
        for ( int i = 0; i < 200; i++ ) {
            docs.add( new Document( "name", "MongoDB" ).append( "count", i )
                    .append( "versions",
                            Arrays.asList( "v3.2", "v3.0", "v2.6" ) )
                    .append( "info",
                            new Document( "x", i * 2 ).append( "y", i * 3 ) ) );
        }
        cl.insertMany( docs );

        MongoCursor< Document > cur = cl.find().batchSize( 10 ).iterator();
        while ( cur.hasNext() ) {
            Document doc = cur.next();
        }

        cur = cl.find( new Document( "count", new Document( "$gt", 100 ) ) )
                .batchSize( 10 ).iterator();
        cur.tryNext();
        cur.close();

        // create collections and indexes
        String clName1 = collectionName + "_1";
        String clName2 = collectionName + "_2";
        String clName3 = collectionName + "_3";
        String idxName1 = "idx_32611_1";
        String idxName2 = "idx_32611_2";
        String idxName3 = "idx_32611_3";

        database.createCollection( clName1 );
        MongoCollection< Document > cl1 = database.getCollection( clName1 );
        cl1.createIndex( new Document( "a", 1 ),
                new IndexOptions().name( idxName1 ) );

        database.createCollection( clName2 );
        MongoCollection< Document > cl2 = database.getCollection( clName2 );
        cl2.createIndex( new Document( "b", 1 ).append( "c", -1 ),
                new IndexOptions().name( idxName2 ) );

        database.createCollection( clName3, new CreateCollectionOptions()
                .capped( true ).sizeInBytes( 1000000 ) );
        MongoCollection< Document > cl3 = database.getCollection( clName3 );
        cl3.createIndex( new Document( "d", 1 ), new IndexOptions()
                .name( idxName3 ).unique( true ).background( true ) );

        // drop indexes and collections
        cl1.dropIndex( idxName1 );
        cl2.dropIndex( idxName2 );
        cl3.dropIndex( idxName3 );

        cl1.drop();
        cl2.drop();
        cl3.drop();

        // stop sniffer and analyze the message file
        CommLib.snifferStopAndAnalyze( ssh );

        // 分析结果校验
        ssh.exec( "cat " + snifferOutputPath + "*.json" );
        JSONArray messages = getJsonArray( ssh.getStdout() );
        for ( int i = 0; i < messages.size(); i++ ) {
            JSONObject msg = messages.getJSONObject( i );
            System.out.println( msg.toString() );

            switch ( msg.getString( "databaseCmd" ) ) {
            case "getMore":
                checkGetMoreInfo( msg );
                break;
            case "killCursors":
                checkKillCursorsInfo( msg );
                break;
            case "create":
                checkCreateInfo( msg );
                break;
            case "createIndexes":
                checkCreateIndexesInfo( msg );
                break;
            case "dropIndexes":
                checkDropIndexesInfo( msg );
                break;
            case "drop":
                checkDropInfo( msg );
                break;
            default:
                continue;
            }
        }
    }

    @AfterClass
    public void teardown() throws Exception {
        CommLib.rmDir( ssh, snifferOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
        if ( mongoClient != null )
            mongoClient.close();
        if ( ssh != null )
            ssh.disconnect();
    }

    // 输入的str有多行，每行一个json，返回一个json数组
    JSONArray getJsonArray( String str ) {
        JSONArray jsonArray = new JSONArray();
        String[] lines = str.split( "\n" );
        for ( String line : lines ) {
            jsonArray.add( JSONObject.parseObject( line ) );
        }
        return jsonArray;
    }

    void checkGetMoreInfo( JSONObject msg ){
        Param msgParam = new Param();

        ArrayList< Param > subParam = new ArrayList<>();
        subParam.add( new Param( "batchSize", 20 ) );
        subParam.add( new Param( "getMore", 20 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParam );

        Param opParam = new Param();
        Param.checkRecordParams( msg, "getMore", 20, msgParam, dbParam,
                opParam );
    }

    void checkKillCursorsInfo( JSONObject msg ){
        Param msgParam = new Param();

        ArrayList< Param > subParam = new ArrayList<>();
        subParam.add( new Param( "killCursors", 1 ) );
        subParam.add( new Param( "cursors", 1 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParam );

        Param opParam = new Param();
        Param.checkRecordParams( msg, "killCursors", 1, msgParam, dbParam,
                opParam );
    }

    void checkCreateInfo( JSONObject msg ){
        Param msgParam = new Param();

        ArrayList< Param > subParam = new ArrayList<>();
        subParam.add( new Param( "size", 1 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParam );

        Param opParam = new Param();
        Param.checkRecordParams( msg, "create", 4, msgParam, dbParam,
                opParam );
    }

    void checkCreateIndexesInfo( JSONObject msg ){
        Param msgParam = new Param();

        ArrayList< Param > subParam = new ArrayList<>();
        subParam.add( new Param( "indexes.ns", 3 ) );
        subParam.add( new Param( "indexes.background", 1 ) );
        subParam.add( new Param( "indexes.unique", 1 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParam );

        Param opParam = new Param();
        Param.checkRecordParams( msg, "createIndexes", 3, msgParam, dbParam,
                opParam );
    }

    void checkDropIndexesInfo( JSONObject msg ){
        Param msgParam = new Param();

        ArrayList< Param > subParam = new ArrayList<>();
        subParam.add( new Param( "dropIndexes", 3 ) );
        subParam.add( new Param( "index", 3 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParam );

        Param opParam = new Param();
        Param.checkRecordParams( msg, "dropIndexes", 3, msgParam, dbParam,
                opParam );
    }

    void checkDropInfo( JSONObject msg ){
        Param msgParam = new Param();

        ArrayList< Param > subParam = new ArrayList<>();
        subParam.add( new Param( "drop", 3 ) );
        Param dbParam = new Param( "databaseCmdParameters", 0,
                subParam );

        Param opParam = new Param();
        Param.checkRecordParams( msg, "drop", 3, msgParam, dbParam,
                opParam );
    }
}
