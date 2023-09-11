package com.mongodb.springdata;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;

import org.bson.BsonTimestamp;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.CommandResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33052:CRUD操作timestamp()数据
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class DateTypeTimestamp33052 extends MongodbTestBase {
    private String clName;

    @BeforeClass
    private void setUp() {
        clName = springDBNameWithVersion + "_cl_33052";
        mongoTemplate.dropCollection( clName );
        mongoTemplate.createCollection( clName );

        // 插入BsonTimestamp()
        // 补充说明：springdata mongo 不会将 new BsonTimestamp(0,0) 转换为当前时间，同 mongo引擎
        List< MyEntity > docs = new ArrayList<>();
        docs.add( new MyEntity( 0, new BsonTimestamp() ) );
        docs.add( new MyEntity( 1, new BsonTimestamp(
                Calendar.getInstance().getTime().getTime() ) ) );
        docs.add( new MyEntity( 2, new BsonTimestamp( 0, 0 ) ) );
        mongoTemplate.insert( docs, clName );
    }

    @Test
    private void test() {
        // 获取mongo服务器本地时间
        CommandResult commandResult = mongoTemplate
                .executeCommand( new BasicDBObject( "serverStatus", 1 ) );
        long localTimeMS = commandResult.getDate( "localTime" ).getTime();

        // 检查结果
        List< MyEntity > actDocs = mongoTemplate.find(
                new Query( Criteria.where( "_id" ).lte( 2 ) ), MyEntity.class,
                clName );
        // 同mongo引擎测试结果
        Assert.assertEquals( actDocs.get( 1 ).getId(), 1 );
        // 当前时间实时变化，校验结果容许1小时误差值
        long actTimeMS = actDocs.get( 1 ).getA().getValue();
        Assert.assertTrue(
                Math.abs( localTimeMS - actTimeMS ) < 1 * 3600 * 1000,
                localTimeMS + ", " + actTimeMS + ", "
                        + Math.abs( localTimeMS - actTimeMS ) );
        // 同mongo引擎测试结果
        Assert.assertEquals( actDocs.get( 0 ).getId(), 0 );
        Assert.assertEquals( actDocs.get( 0 ).getA().getValue(), 0 );
        Assert.assertEquals( actDocs.get( 2 ).getId(), 2 );
        Assert.assertEquals( actDocs.get( 2 ).getA().getValue(), 0 );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }

    private class MyEntity implements Serializable {
        private static final long serialVersionUID = 1L;
        private int id;
        private BsonTimestamp a;

        private MyEntity( int id, BsonTimestamp a ) {
            super();
            this.id = id;
            this.a = a;
        }

        private int getId() {
            return id;
        }

        private BsonTimestamp getA() {
            return a;
        }
    }
}
