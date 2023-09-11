package com.mongodb.java;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

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
import com.mongodb.client.model.Filters;
import com.mongodb.client.model.Updates;
import com.mongodb.client.result.UpdateResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33055:update使用$currentDate操作符
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class UpdatesCurrentDate33055 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;

    @BeforeClass
    private void setUp() {
        clName = javaDBNameWithVersion + "_cl_33055";
        db = MongodbTestBase.getDataBase( client );
        cl = db.getCollection( clName );
        cl.drop();
        db.createCollection( clName );
        List< Document > docs = new ArrayList<>();
        for ( int i = 1; i <= 6; i++ ) {
            docs.add( new Document( "_id", i ).append( "a", i ) );
        }
        cl.insertMany( docs );
    }

    @Test
    private void test() {
        Bson filter;
        UpdateResult updateResult;

        // 获取mongo服务器本地时间
        Document commandResult = db
                .runCommand( new Document( "serverStatus", 1 ) );
        long localTimeMS = commandResult.getDate( "localTime" ).getTime();

        // updateOne
        filter = Filters.eq( "_id", 1 );
        updateResult = cl.updateOne( filter,
                Updates.combine( Updates.currentDate( "a" ) ) );
        Assert.assertEquals( updateResult.getModifiedCount(), 1 );
        this.checkResult( filter, localTimeMS );

        // updateMany
        filter = Filters.lte( "_id", 3 );
        updateResult = cl.updateMany( filter,
                Updates.combine( Updates.currentDate( "a" ) ) );
        Assert.assertEquals( updateResult.getModifiedCount(), 3 );
        this.checkResult( filter, localTimeMS );

        // update
        filter = Filters.lte( "_id", 5 );
        updateResult = cl.updateMany( filter,
                Updates.combine( Updates.currentDate( "a" ) ) );
        Assert.assertEquals( updateResult.getModifiedCount(), 5 );
        this.checkResult( filter, localTimeMS );

        // findOneAndUpdate
        filter = Filters.eq( "_id", 6 );
        Document findAndUpdateResult = cl.findOneAndUpdate( filter,
                Updates.combine( Updates.currentDate( "a" ) ) );
        Assert.assertEquals( findAndUpdateResult,
                new Document( "_id", 6 ).append( "a", 6 ) );
        this.checkResult( filter, localTimeMS );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }

    private void checkResult( Bson filter, long localTimeMS ) {
        MongoCursor< Document > cursor = cl.find( filter ).iterator();
        while ( cursor.hasNext() ) {
            Document doc = cursor.next();
            long actTimeMS = doc.get( "a", Date.class ).getTime();
            // 当前时间实时变化，校验结果容许300秒误差值
            Assert.assertTrue( Math.abs( localTimeMS - actTimeMS ) < 300 * 1000,
                    localTimeMS + ", " + actTimeMS + ", "
                            + Math.abs( localTimeMS - actTimeMS ) );
        }
        cursor.close();
    }
}
