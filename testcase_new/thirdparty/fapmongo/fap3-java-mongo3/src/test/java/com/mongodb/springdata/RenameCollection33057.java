package com.mongodb.springdata;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.DBCollection;
import com.mongodb.MongoCommandException;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33057:renameCollection接口功能验证
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class RenameCollection33057 extends MongodbTestBase {
    private String clName;
    private String newCLName = clName + ".new";

    @BeforeClass
    private void setUp() {
        clName = springDBNameWithVersion + "_cl_33057";
        mongoTemplate.dropCollection( clName );
        mongoTemplate.createCollection( clName );
        mongoTemplate.createCollection( newCLName );
        mongoTemplate.insert( new Document( "_id", 1 ).append( "a", 1 ),
                clName );
    }

    @Test
    private void test() {
        DBCollection newCL = mongoTemplate.getCollection( newCLName );
        Assert.assertEquals( mongoTemplate.count( null, clName ), 1 );
        Assert.assertEquals( mongoTemplate.count( null, newCLName ), 0 );

        // dropTarget( false )
        try {
            newCL.rename( clName, false );
            Assert.fail( "expect fail but actual success." );
        } catch ( MongoCommandException e ) {
            if ( -22 != e.getErrorCode() )
                throw e;
        }
        // check result
        Assert.assertEquals( mongoTemplate.count( null, clName ), 1 );
        Assert.assertEquals( mongoTemplate.count( null, newCLName ), 0 );

        // SEQUOIADBMAINSTREAM-9898，dropTrage:true时fapmongo不支持删除目标表
        try {
            newCL.rename( clName, true );
            Assert.fail( "expect fail but actual success." );
        } catch ( MongoCommandException e ) {
            if ( -22 != e.getErrorCode() )
                throw e;
        }
        // check result
        Assert.assertEquals( mongoTemplate.count( null, clName ), 1 );
        Assert.assertEquals( mongoTemplate.count( null, newCLName ), 0 );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
        dropCLByTestResult( context, this.toString(), mongoTemplate,
                newCLName );
    }
}
