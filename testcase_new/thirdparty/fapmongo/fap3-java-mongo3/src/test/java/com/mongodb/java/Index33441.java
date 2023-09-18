package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoCursor;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Indexes;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-33441:按key删索引
 *               cl.dropIndexes只能带配置参数删除所有索引，不能指定索引名或key删除多个索引
 * @author Xiaoni Huang
 * @Date:2023/9/18
 */
public class Index33441 extends MongodbTestBase {
    private MongoDatabase db;
    private MongoCollection< Document > cl;
    private String clName;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl_33441";
        db.createCollection( clName );
        cl = db.getCollection( clName );
        cl.createIndex( Indexes.ascending( "a" ) );
        cl.createIndex( Indexes.ascending( "b" ) );
    }

    @Test
    public void test() {
        // 按key删除索引
        cl.dropIndex( new Document( "a", 1 ) );
        MongoCursor< Document > cursor = cl.listIndexes().iterator();
        int actIdxNum = 0;
        while ( cursor.hasNext() ) {
            String idxName = cursor.next().getString( "name" );
            if ( idxName.equals( "a_1" ) )
                Assert.fail( "check indexes fail." );
            actIdxNum++;
        }
        cursor.close();
        Assert.assertEquals( actIdxNum, 2 );

        // 批量删除索引（不包括 _id_ 索引）
        cl.dropIndexes();
        int actIdxNum2 = cl.listIndexes().into( new ArrayList< Document >() )
                .size();
        Assert.assertEquals( actIdxNum2, 1 );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
