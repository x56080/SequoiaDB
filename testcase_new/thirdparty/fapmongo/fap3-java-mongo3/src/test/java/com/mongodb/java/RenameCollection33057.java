package com.mongodb.java;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.MongoCommandException;
import com.mongodb.MongoNamespace;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.RenameCollectionOptions;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33057:renameCollection接口功能验证
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class RenameCollection33057 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private String newCLName = clName + ".new";
    private MongoCollection< Document > cl;

    @BeforeClass
    private void setUp() {
        clName = javaDBNameWithVersion + "_cl_33057";
        db = MongodbTestBase.getDataBase( client );
        cl = db.getCollection( clName );
        cl.drop();
        db.createCollection( clName );
        cl.insertOne( new Document( "_id", 1 ).append( "a", 1 ) );
    }

    @Test
    private void test_find() {
        db.createCollection( newCLName );
        MongoCollection< Document > newCL = db.getCollection( newCLName );
        Assert.assertEquals( cl.count(), 1 );
        Assert.assertEquals( newCL.count(), 0 );

        // dropTarget( false )
        try {
            newCL.renameCollection( new MongoNamespace( db.getName(), clName ),
                    new RenameCollectionOptions().dropTarget( false ) );
            Assert.fail( "expect fail but actual success." );
        } catch ( MongoCommandException e ) {
            if ( -22 != e.getErrorCode() )
                throw e;
        }
        // check result
        Assert.assertEquals( cl.count(), 1 );
        Assert.assertEquals( newCL.count(), 0 );

        // SEQUOIADBMAINSTREAM-9898，dropTrage:true时fapmongo不支持删除目标表
        try {
            newCL.renameCollection( new MongoNamespace( db.getName(), clName ),
                    new RenameCollectionOptions().dropTarget( true ) );
            Assert.fail( "expect fail but actual success." );
        } catch ( MongoCommandException e ) {
            if ( -22 != e.getErrorCode() )
                throw e;
        }
        // check result
        Assert.assertEquals( cl.count(), 1 );
        Assert.assertEquals( newCL.count(), 0 );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
        dropCLByTestResult( context, this.toString(), db, newCLName );
    }
}
