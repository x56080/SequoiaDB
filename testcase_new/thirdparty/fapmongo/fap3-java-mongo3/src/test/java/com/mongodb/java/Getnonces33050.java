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
 * @Description seqDB-33050:getnonce获取身份验证随机数
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class Getnonces33050 extends MongodbTestBase {
    private MongoDatabase db;

    @BeforeClass
    private void setUp() {
        db = MongodbTestBase.getDataBase( client );
    }

    @Test
    private void test() {
        Document rt = db.runCommand( new Document( "getnonce", 1 ) );
        Assert.assertEquals( rt.toString(),
                new Document( "nonce", 0 ).append( "ok", 1 ).toString() );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
    }
}
