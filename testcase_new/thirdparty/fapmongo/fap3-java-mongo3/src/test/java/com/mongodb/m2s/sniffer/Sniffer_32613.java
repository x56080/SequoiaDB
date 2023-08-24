package com.mongodb.m2s.sniffer;

import com.mongodb.client.*;
import com.mongodb.m2s.testcommon.CommLib;
import com.mongodb.m2s.testcommon.M2STestBase;
import com.mongodb.utils.Ssh;
import org.bson.Document;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Arrays;

/**
 * @Descreption seqDB-32613:开启用户数据过滤，检测消息报告
 * @Author tangtao
 * @CreateDate
 * @UpdateUser
 * @UpdateDate 2023/8/22
 * @UpdateRemark
 * @Version
 */

public class Sniffer_32613 extends M2STestBase {
    private MongoClient mongoClient = null;
    private Ssh ssh = null;
    private String databaseName = "db_32613";
    private String collectionName = "coll_32613";

    @BeforeClass
    public void setup() throws Exception {
        mongoClient = MongoClients.create( mongodbUri );
        ssh = new Ssh( remoteHost, remoteUser, remotePwd );
        CommLib.initDir( ssh, snifferOutputPath );
        mongoClient.getDatabase( databaseName ).drop();
    }

    @Test
    public void test() throws Exception {

        String capturePath = toolRootPath + "m2s-sniffer/capture/*";
        ssh.exec( "rm -rf " + capturePath );

        String snifferCommand = snifferPath + " server-start -l "
                + snifferListenPort + " -m " + snifferAddr
                + " -r without-userdata";
        ssh.exec( snifferCommand );

        // create collection
        MongoClient snifferClient = CommLib.getSnifferClient();
        MongoDatabase database = snifferClient.getDatabase( databaseName );

        database.createCollection( collectionName );
        MongoCollection< Document > cl = database
                .getCollection( collectionName );

        // insert data
        for ( int i = 0; i < 10; i++ ) {
            cl.insertOne( new Document( "name", "MongoDB" ).append( "count", i )
                    .append( "versions",
                            Arrays.asList( "v3.2", "v3.0", "v2.6" ) )
                    .append( "info",
                            new Document( "x", i * 2 ).append( "y", i * 3 ) ) );
        }

        // stop sniffer
        snifferCommand = snifferPath + " server-stop";
        ssh.exec( snifferCommand );

        String getReportCommand = "find " + toolRootPath
                + "m2s-sniffer/capture/ -name \"*.log\" -type f -print -quit | xargs cat ";
        ssh.exec( getReportCommand );
        Assert.assertTrue(
                ssh.getStdout().contains( "{\"snifferRecordWithoutUserData\":true}" ),
                "sniffer tool did not replace userdata" );
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
}
