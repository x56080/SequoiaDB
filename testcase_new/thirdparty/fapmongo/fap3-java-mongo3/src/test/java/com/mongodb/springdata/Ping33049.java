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
 * @Description seqDB-33049:ping操作
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class Ping33049 extends MongodbTestBase {

    @BeforeClass
    private void setUp() {
    }

    @Test
    private void test() {
        CommandResult commandResult = mongoTemplate
                .executeCommand( new BasicDBObject( "ping", 1 ) );
        Assert.assertTrue( commandResult.ok() );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
    }
}
