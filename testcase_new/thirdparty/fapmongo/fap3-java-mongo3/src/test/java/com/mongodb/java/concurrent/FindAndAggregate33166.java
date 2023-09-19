package com.mongodb.java.concurrent;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.bson.Document;
import org.bson.conversions.Bson;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoCursor;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Accumulators;
import com.mongodb.client.model.Aggregates;
import com.mongodb.client.model.Sorts;
import com.mongodb.utils.MongodbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-33166:find/aggregate大数据量并发测试
 * @author XiaoNi Huang
 * @Date 2023/9/4
 */
public class FindAndAggregate33166 extends MongodbTestBase {
    private static final Logger logger = LogManager.getLogger( "" );
    private MongoDatabase db;
    private MongoCollection< Document > cl;
    private String clName;
    private int docsNum = 50000;
    private int insertBatchSize = 50000;

    @BeforeClass
    private void setUp() {
        db = MongodbTestBase.getDataBase( client );
        clName = javaDBNameWithVersion + "_cl_33166";
        cl = db.getCollection( clName );
        cl.drop();
        // 准备数据，分批插入
        for ( int k = 0; k < docsNum; k += insertBatchSize ) {
            List< Document > batchDocs = new ArrayList<>();
            for ( int i = k; i < k + insertBatchSize; i++ ) {
                batchDocs.add( new Document( "_id", i ).append( "a", i ) );
            }
            cl.insertMany( batchDocs );
        }
    }

    @Test
    private void test() throws Exception {
        // 并发执行find和aggregate
        int threadNum = 10;
        ThreadExecutor threadExec = new ThreadExecutor();
        for ( int i = 0; i < threadNum; i++ ) {
            threadExec.addWorker( new FindThread() );
            threadExec.addWorker( new AggregateThread() );
        }
        threadExec.run();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }

    private class FindThread {
        @ExecuteOrder(step = 1)
        private void test() {
            MongoDatabase db = MongodbTestBase
                    .getDataBase( MongodbTestBase.client );
            MongoCollection< Document > cl = db.getCollection( clName );
            logger.info( javaDBNameWithVersion + " FindThread begin" );
            MongoCursor< Document > cursor = cl.find().iterator();
            int expDocsNum = 0;
            while ( cursor.hasNext() ) {
                cursor.next();
                expDocsNum++;
            }
            cursor.close();
            Assert.assertEquals( expDocsNum, docsNum );
            logger.info( javaDBNameWithVersion + " FindThread end" );
        }
    }

    private class AggregateThread {
        @ExecuteOrder(step = 1)
        private void test() {
            MongoDatabase db = MongodbTestBase
                    .getDataBase( MongodbTestBase.client );
            MongoCollection< Document > cl = db.getCollection( clName );

            List< Bson > agg = Arrays.asList(
                    Aggregates.group( "$_id",
                            Accumulators.sum( "sum_a", "$a" ) ),
                    Aggregates.sort( Sorts.ascending( "_id" ) ) );
            logger.info( javaDBNameWithVersion + " AggregateThread begin" );
            MongoCursor< Document > cursor = cl.aggregate( agg ).iterator();
            int expDocsNum = 0;
            while ( cursor.hasNext() ) {
                cursor.next();
                expDocsNum++;
            }
            Assert.assertEquals( expDocsNum, docsNum );
            cursor.close();
            logger.info( javaDBNameWithVersion + " AggregateThread end" );
        }
    }
}
