package com.mongodb.springdata;

import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.UncategorizedMongoDbException;
import org.springframework.data.mongodb.core.index.Index;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DBObject;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21931:count操作
 * @author fanyu
 * @Date 2020/3/19
 * @version 1.00
 */
public class Count21931 extends MongodbTestBase {
    private String clName = springDBNameWithVersion + "_cl21931";
    // 不能小于10
    private int num = 10;

    @BeforeClass
    public void setUp() {
        List< DBObject > list = new CopyOnWriteArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", "" + i )
                    .append( "c", new int[] { i + 1, i + 2, i + 3 } )
                    .append( "d", "test-" + i + "-" + i % 3 ) );
        }
        mongoTemplate.insert( list, clName );
        mongoTemplate.insert( list, Count21931.class );
        mongoTemplate.indexOps( clName ).ensureIndex(
                new Index().named( "a" ).on( "a", Sort.Direction.ASC ) );
    }

    @Test
    public void test1() {
        Query query;
        long actCount;
        // 带空条件count
        actCount = mongoTemplate.count( new Query(), clName );
        Assert.assertEquals( actCount, num );

        // 带条件is + lt + gte
        query = new Query( Criteria.where( "a" ).gte( 0 ).lte( num / 2 ) );
        actCount = mongoTemplate.count( query, clName );
        Assert.assertEquals( actCount, num / 2 + 1 );

        // 带条件in + hint
        // spring data hint只有指定索引名的方式，无键值对的方式
        // 索引存在，hint为索引名
        query = new Query( Criteria.where( "b" ).in( "1", "2", "3" ) );
        query.withHint( "a" );
        actCount = mongoTemplate.count( query, clName );
        Assert.assertEquals( actCount, 3 );

        // 索引不存在，hint为索引名
        query = new Query( Criteria.where( "b" ).in( "1", "2", "3" ) );
        query.withHint( "a-inextences" );
        actCount = mongoTemplate.count( query, clName );
        Assert.assertEquals( actCount, 3 );

        // 集合不存在数据，进行count
        mongoTemplate.remove( new Query(), clName );
        actCount = mongoTemplate.count( new Query(), clName );
        Assert.assertEquals( actCount, 0 );

        // 参数校验
        try {
            mongoTemplate.count( new Query( Criteria.where( "$gt" ).is( 1 ) ),
                    clName );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "-6" ) ) {
                throw e;
            }
        }
    }

    @Test
    public void test2() {
        Query query;
        long actCount;
        // 详细验证由test1进行验证，这里只是简单的验证
        // 带空条件count
        actCount = mongoTemplate.count( new Query(), Count21931.class );
        Assert.assertEquals( actCount, num );

        // 带条件count
        query = new Query( Criteria.where( "a" ).gte( 0 ).lte( num / 2 ) );
        actCount = mongoTemplate.count( query, Count21931.class );
        Assert.assertEquals( actCount, num / 2 + 1 );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }
}
