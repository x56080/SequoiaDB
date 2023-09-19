package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.BulkWriteOptions;
import com.mongodb.client.model.InsertOneModel;
import com.mongodb.client.model.WriteModel;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-33194:bulkwrite ordere:false 写入大批量记录，部分记录冲突
 * @Author XiaoNi Huang
 * @Date 2023/09/06
 */
public class BulkwriteOrdered33194 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;
    private int docsNum = 50000;

    @BeforeClass
    private void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl_33194";
        cl = db.getCollection( clName );
        cl.drop();
    }

    @Test
    private void test() {
        // 准备要写入的数据和预期数据
        List< WriteModel< Document > > writeMode1 = new ArrayList<>();
        List< WriteModel< Document > > writeMode2 = new ArrayList<>();
        List< Document > expDocs1 = new ArrayList<>();
        List< Document > expDocs2 = new ArrayList<>();
        for ( int i = 0; i < docsNum; i++ ) {
            Document doc = new Document( "_id", i ).append( "a", i );

            // 准备首次要写入的数据（被10整除的数据，用于后续bulkwrite时的冲突数据）
            if ( i % 10 == 0 ) {
                InsertOneModel< Document > insertOneMode1 = new InsertOneModel< Document >(
                        doc );
                writeMode1.add( insertOneMode1 );
                expDocs1.add( doc );
            }

            // 准备二次要写入的全量数据
            InsertOneModel< Document > insertOneMode2 = new InsertOneModel<>(
                    doc );
            writeMode2.add( insertOneMode2 );
            expDocs2.add( doc );
        }
        Assert.assertEquals( expDocs1.size(), docsNum / 10 );
        Assert.assertEquals( expDocs2.size(), docsNum );

        // 首次写入一部分数据
        cl.bulkWrite( writeMode1 );
        // 检查写入后的数据
        List< Document > actDocs1 = cl.find().sort( new Document( "_id", 1 ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actDocs1, expDocs1 );

        // 批量插入所有数据（其中包括冲突的记录）
        // ordered: false
        cl.bulkWrite( writeMode2, new BulkWriteOptions().ordered( false ) );
        // 检查写入后的数据
        List< Document > actDocs2 = cl.find().sort( new Document( "_id", 1 ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actDocs2.size(), expDocs2.size() );
        Assert.assertEquals( actDocs2, expDocs2 );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
