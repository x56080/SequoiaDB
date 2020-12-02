package com.mongodb.springdata;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.core.query.Update;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.mongodb.DBCollection;
import com.mongodb.WriteResult;
import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21996:find/update/distinct/aggregate/count/delete大量数据
 * @author fanyu
 * @Date 2020/3/31
 * @version 1.00
 */
public class AllOperation21996 extends MongodbTestBase {
    private String clNameBase = "spring_cl21996B";
    private AtomicInteger clNum = new AtomicInteger( 5 );

    @BeforeClass
    public void setUp() {
    }

    @DataProvider(name = "data-provider", parallel = true)
    private Object[][] rangeData() {
        return new Object[][] {
                { clNameBase + String.valueOf( clNum.getAndDecrement() ), 999 },
                { clNameBase + String.valueOf( clNum.getAndDecrement() ),
                        1000 },
                { clNameBase + String.valueOf( clNum.getAndDecrement() ),
                        3000 } };
    }

    @SuppressWarnings("rawtypes")
    @Test(dataProvider = "data-provider", enabled = false) // jira-6463
    public void test1( String clName, int recordNum ) {
        List< Entity > list = new ArrayList<>();
        for ( int i = 0; i < recordNum; i++ ) {
            list.add(
                    new Entity( "a" + i, Entity.SEXS[ i % Entity.SEXS.length ],
                            i, i, Entity.COURSES ) );
        }
        mongoTemplate.insert( list, clName );

        // find
        List< Entity > list1 = mongoTemplate.findAll( Entity.class, clName );
        Assert.assertEquals( list1, list );

        // find limit 999
        Query query = new Query( Criteria.where( "age" ).gt( -1 ) );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        query.limit( 999 );
        List< Entity > list2 = mongoTemplate.find( query, Entity.class,
                clName );
        Assert.assertEquals( list2,
                list.subList( 0, Math.min( 999, recordNum ) ) );

        // find limit 1000
        query.limit( 1000 );
        List< Entity > list3 = mongoTemplate.find( query, Entity.class,
                clName );
        Assert.assertEquals( list3,
                list.subList( 0, Math.min( 1000, recordNum ) ) );

        // find limit 1001
        query.limit( 1001 );
        List< Entity > list4 = mongoTemplate.find( query, Entity.class,
                clName );
        Assert.assertEquals( list4,
                list.subList( 0, Math.min( 1001, recordNum ) ) );

        // count
        Assert.assertEquals( mongoTemplate.count( new Query(), clName ),
                recordNum );
        Assert.assertEquals(
                mongoTemplate.count( new Query( Criteria.where( "age" )
                        .gte( recordNum / 2 ).lte( recordNum ) ), clName ),
                recordNum - recordNum / 2 );

        // distinct
        DBCollection cl = mongoTemplate.getCollection( clName );
        List list5 = cl.distinct( "age" );
        Assert.assertEquals( list5.size(), recordNum );

        // update
        WriteResult actWriteResult = mongoTemplate.updateMulti( new Query(),
                Update.update( "age", recordNum ), Entity.class, clName );
        Assert.assertEquals( actWriteResult.getN(), recordNum );
        Assert.assertTrue( actWriteResult.isUpdateOfExisting() );
        Assert.assertNull( actWriteResult.getUpsertedId() );
        Assert.assertEquals( mongoTemplate.count(
                new Query( Criteria.where( "age" ).is( recordNum ) ), clName ),
                recordNum );

        // aggregate
        // 匹配记录数超过1000，聚集操作返回的结果不正确，
        // 与开发确认，暂时不在2.14.2版本上解决
        // Aggregation agg1 = Aggregation.newAggregation(
        // match( Criteria.where( "age" ).lte( recordNum ) ) );
        // AggregationResults< String > results1 = mongoTemplate.aggregate(
        // agg1,
        // clName, String.class );
        // List< String > act1 = results1.getMappedResults();
        // Assert.assertEquals( act1.size(), recordNum );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = new String[] { clNameBase + 3, clNameBase + 2,
                clNameBase + 1 };
        dropCLByTestResult( context, this.toString(), mongoTemplate, clNames );
    }
}
