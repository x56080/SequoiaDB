package com.mongodb.java;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoDatabase;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-32898:ping检查连接状态
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class Ping33049 extends MongodbTestBase {
    private MongoDatabase db;

    @BeforeClass
    private void setUp() {
        db = MongodbTestBase.getDataBase( client );
    }

    @Test
    private void test() {
        Document rt = db.runCommand( new Document( "ping", 1 ) );
        Assert.assertEquals( rt.toString(),
                new Document( "ok", 1 ).toString() );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
    }
}
