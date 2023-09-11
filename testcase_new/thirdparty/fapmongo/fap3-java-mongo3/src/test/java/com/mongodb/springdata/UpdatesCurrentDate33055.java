package com.mongodb.springdata;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.core.query.Update;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.CommandResult;
import com.mongodb.WriteResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-33055:update使用$currentDate操作符
 * @author XiaoNi Huang
 * @Date 2023/8/17
 */
public class UpdatesCurrentDate33055 extends MongodbTestBase {
    private String clName;

    @BeforeClass
    private void setUp() {
        clName = springDBNameWithVersion + "_cl_33055";
        mongoTemplate.dropCollection( clName );
        mongoTemplate.createCollection( clName );

        List< MyEntity > docs = new ArrayList<>();
        for ( int i = 0; i < 6; i++ ) {
            docs.add( new MyEntity( i, new Date( i ) ) );
        }
        mongoTemplate.insert( docs, clName );

    }

    @Test
    private void test() {
        // 获取mongo服务器本地时间
        CommandResult commandResult = mongoTemplate
                .executeCommand( new BasicDBObject( "serverStatus", 1 ) );
        long localTimeMS = commandResult.getDate( "localTime" ).getTime();

        // update
        Query query = new Query( Criteria.where( "_id" ).lte( 3 ) );
        Update update = new Update().currentDate( "a" );
        WriteResult writeResult = mongoTemplate.updateMulti( query, update,
                clName );
        Assert.assertEquals( writeResult.getN(), 4 );
        this.checkResult( query, localTimeMS );
    }

    @AfterClass
    private void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }

    private void checkResult( Query query, long localTimeMS ) {
        List< MyEntity > actDocs = mongoTemplate.find( query, MyEntity.class,
                clName );
        // 当前时间实时变化，校验结果容许1小时秒误差值
        for ( MyEntity doc : actDocs ) {
            long actTimeMS = doc.getA().getTime();
            Assert.assertTrue(
                    Math.abs( localTimeMS - actTimeMS ) < 1 * 3600 * 1000,
                    localTimeMS + ", " + actTimeMS + ", "
                            + Math.abs( localTimeMS - actTimeMS ) );
        }
    }

    private class MyEntity implements Serializable {
        private static final long serialVersionUID = 1L;
        private int id;
        private Date a;

        private MyEntity( int id, Date a ) {
            super();
            this.id = id;
            this.a = a;
        }

        @SuppressWarnings("unused")
        private int getId() {
            return id;
        }

        private Date getA() {
            return a;
        }
    }
}
