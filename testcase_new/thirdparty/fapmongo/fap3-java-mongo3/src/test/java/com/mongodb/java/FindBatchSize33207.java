package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoCursor;
import com.mongodb.client.MongoDatabase;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-33207:find batchSize大数据量测试
 * @Author XiaoNi Huang
 * @Date 2023/09/11
 */
public class FindBatchSize33207 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;
    private int docsNum = 4 * 50000;

    @BeforeClass
    private void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl_33207";
        cl = db.getCollection( clName );
        cl.drop();
        // 准备数据
        for ( int k = 0; k < docsNum; k += 50000 ) {
            List< Document > docs = new ArrayList<>();
            for ( int i = k; i < k + 50000; i++ ) {
                docs.add( new Document( "_id", i ).append( "a", i ) );
            }
            cl.insertMany( docs );
        }
    }

    @DataProvider(name = "batchSize-data-provider", parallel = true)
    private Object[][] rangeData() {
        return new Object[][] { { 0 }, { 10 }, { 3333 }, { 10000 } };
    }

    @Test(dataProvider = "batchSize-data-provider")
    private void test( int batchSize ) {
        MongoCursor< Document > cursor = cl.find()
                .sort( new Document( "_id", 1 ) ).batchSize( batchSize )
                .iterator();
        // 校验查询结果
        Document doc = null;
        for ( int i = 0; i < docsNum; i++ ) {
            // 校验在预期 docsNum 范围内时 cursor.hasNext() 为 true，避免少数据
            if ( !cursor.hasNext() ) {
                Assert.fail( "batchSize:" + batchSize + ", i:" + i );
            } else {
                doc = cursor.next();
                // 校验最后一条数据
                if ( !cursor.hasNext() )
                    Assert.assertEquals( doc.toString(),
                            new Document( "_id", docsNum - 1 )
                                    .append( "a", docsNum - 1 ).toString() );
            }
            // 校验多数据的情况，查询返回的预期数据多于实际数据
            if ( i == docsNum - 1 && cursor.hasNext() )
                Assert.fail( "多数据，第一条多的数据：" + doc.toString() );

        }
        cursor.close();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
