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
import com.mongodb.client.model.InsertManyOptions;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-33197:insert ordere:false 写入大批量记录，部分记录冲突
 * @Author XiaoNi Huang
 * @Date 2023/09/07
 */
public class InsertOrdered33197 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;
    private int docsNum = 50000;

    @BeforeClass
    private void setUp() throws UnknownHostException {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl_33197";
        cl = db.getCollection( clName );
        cl.drop();
    }

    @Test
    private void test() {
        // 准备要写入的数据和预期数据
        List< Document > docs1 = new ArrayList<>();
        List< Document > docs2 = new ArrayList<>();
        for ( int i = 0; i < docsNum; i++ ) {
            Document doc = new Document( "_id", i ).append( "a", i );
            // 准备首次要写入的数据（被10整除的数据，用于后续bulkwrite时的冲突数据）
            if ( i % 10 == 0 ) {
                docs1.add( doc );
            }
            // 准备二次要写入的全量数据
            docs2.add( doc );
        }
        Assert.assertEquals( docs1.size(), docsNum / 10 );
        Assert.assertEquals( docs2.size(), docsNum );

        // 首次写入一部分数据
        cl.insertMany( docs1 );
        // 批量插入所有数据（其中包括冲突的记录），ordered: false
        cl.insertMany( docs2, new InsertManyOptions().ordered( false ) );
        // 检查写入后的数据
        List< Document > actDocs2 = cl.find().sort( new Document( "_id", 1 ) )
                .into( new ArrayList< Document >() );
        Assert.assertEquals( actDocs2.size(), docs2.size() );
        Assert.assertEquals( actDocs2, docs2 );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
