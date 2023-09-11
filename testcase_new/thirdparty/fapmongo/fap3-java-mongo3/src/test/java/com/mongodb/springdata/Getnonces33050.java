package com.mongodb.springdata;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.CommandResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33050:getnonce获取身份验证随机数
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class Getnonces33050 extends MongodbTestBase {

    @BeforeClass
    private void setUp() {
    }

    @Test
    private void test() {
        CommandResult commandResult = mongoTemplate
                .executeCommand( new BasicDBObject( "getnonce", 1 ) );
        Assert.assertTrue( commandResult.ok() );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
    }
}
