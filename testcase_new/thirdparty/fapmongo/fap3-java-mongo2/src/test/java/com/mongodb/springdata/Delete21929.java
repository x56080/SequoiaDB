package com.mongodb.springdata;

import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.regex.Pattern;

import org.springframework.data.mongodb.UncategorizedMongoDbException;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21929:delete操作
 * @author fanyu
 * @Date 2020/3/19
 * @version 1.00
 */
public class Delete21929 extends MongodbTestBase {
    private String clName;
    // 不能小于10
    private int num = 10;
    private List< Entity > list;

    @BeforeClass
    public void setUp() {
        clName = springDBNameWithVersion + "_cl21929";
        list = new CopyOnWriteArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add(
                    new Entity( "a" + i, Entity.SEXS[ i % Entity.SEXS.length ],
                            i, i, Entity.COURSES ) );
        }
        mongoTemplate.insert( list, clName );
        mongoTemplate.insert( list, Delete21929.class );
    }

    @Test
    public void test1() {
        Query query;
        // 删除单条记录
        mongoTemplate.remove( list.get( 0 ), clName );
        Assert.assertEquals( mongoTemplate.count(
                new Query( Criteria.where( "a" ).is( 0 ) ), clName ), 0 );

        // 删除多条记录
        // lt gte
        query = new Query( Criteria.where( "age" ).gte( 1 ).lt( num / 3 ) );
        Assert.assertEquals( mongoTemplate.count( query, clName ),
                num / 3 - 1 );
        mongoTemplate.remove( query, clName );
        Assert.assertEquals( mongoTemplate.count( query, clName ), 0 );

        // $regex
        Pattern pattern = Pattern.compile( "a" + 2 * num / 3 + ".*" );
        query = new Query( Criteria.where( "name" ).regex( pattern ) );
        Assert.assertEquals( mongoTemplate.count( query, clName ), 1 );
        mongoTemplate.remove( query, Delete21929.class, clName );
        Assert.assertEquals( mongoTemplate.count( query, clName ), 0 );

        // $is
        query = new Query( Criteria.where( "age" ).in( 2 * num / 3 + 1,
                2 * num / 3 + 2, 2 * num / 3 + 3 ) );
        Assert.assertTrue(
                mongoTemplate.count( query, Delete21929.class ) > 0 );
        mongoTemplate.remove( query, Delete21929.class );
        Assert.assertEquals( mongoTemplate.count( query, Delete21929.class ),
                0 );

        // 匹配不到记录，删除记录
        query = new Query( Criteria.where( "age" ).gt( num ) );
        Assert.assertEquals( mongoTemplate.count( query, clName ), 0 );
        mongoTemplate.remove( query, clName );

        // 字段不存在，删除记录
        query = new Query( Criteria.where( "k" ).gt( num ) );
        mongoTemplate.remove( query, clName );

        // 带空bson，删除所有记录
        query = new Query();
        Assert.assertTrue( mongoTemplate.count( query, clName ) > 0 );
        mongoTemplate.remove( query, clName );
        Assert.assertEquals( mongoTemplate.count( query, clName ), 0 );

        // 集合不存在记录，删除记录
        mongoTemplate.remove( query, clName );

        // 参数校验
        try {
            mongoTemplate.remove( new Query( Criteria.where( "$" ).is( 0 ) ),
                    clName );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "-6" ) ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }
}
