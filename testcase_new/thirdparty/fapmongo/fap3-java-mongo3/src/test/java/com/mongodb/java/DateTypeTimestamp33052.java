package com.mongodb.java;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BsonTimestamp;
import org.bson.Document;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoCursor;
import com.mongodb.client.MongoDatabase;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33052:CRUD操作timestamp()数据
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class DateTypeTimestamp33052 extends MongodbTestBase {
    private MongoDatabase db;
    private String clName;
    private MongoCollection< Document > cl;

    @BeforeClass
    private void setUp() {
        clName = javaDBNameWithVersion + "_cl_33052";
        db = MongodbTestBase.getDataBase( client );
        cl = db.getCollection( clName );
        cl.drop();
        db.createCollection( clName );
    }

    @Test
    private void test() {
        // 获取mongo服务器本地时间
        Document commandResult = db
                .runCommand( new Document( "serverStatus", 1 ) );
        Date localTime = commandResult.getDate( "localTime" );
        long localTimeS = Math.round( localTime.getTime() / 1000 );

        // 插入BsonTimestamp()
        ArrayList< Document > docs = new ArrayList<>();
        docs.add( new Document( "_id", 1 ).append( "a", new BsonTimestamp() ) );
        docs.add( new Document( "_id", 2 ).append( "a",
                new BsonTimestamp( 0, 0 ) ) );
        cl.insertMany( docs );

        // 检查结果
        Document filter = new Document( "_id", new Document( "$lte", 2 ) );
        MongoCursor< Document > cursor = cl.find( filter ).iterator();
        while ( cursor.hasNext() ) {
            Document doc = cursor.next();
            BsonTimestamp timestamp = doc.get( "a", BsonTimestamp.class );
            long actTimeS = timestamp.getTime();
            // 当前时间实时变化，校验结果容许300秒误差值
            Assert.assertTrue( Math.abs( localTimeS - actTimeS ) < 300,
                    localTimeS + ", " + actTimeS + ", "
                            + Math.abs( localTimeS - actTimeS ) );
        }
        cursor.close();
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), db, clName );
    }
}
