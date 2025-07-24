package com.mongodb.springdata;

import static org.springframework.data.mongodb.core.aggregation.Aggregation.group;
import static org.springframework.data.mongodb.core.aggregation.Aggregation.limit;
import static org.springframework.data.mongodb.core.aggregation.Aggregation.match;
import static org.springframework.data.mongodb.core.aggregation.Aggregation.project;
import static org.springframework.data.mongodb.core.aggregation.Aggregation.skip;
import static org.springframework.data.mongodb.core.aggregation.Aggregation.sort;

import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;

import org.springframework.dao.InvalidDataAccessApiUsageException;
import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.core.aggregation.Aggregation;
import org.springframework.data.mongodb.core.aggregation.AggregationResults;
import org.springframework.data.mongodb.core.index.Index;
import org.springframework.data.mongodb.core.query.Criteria;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21930:aggregate操作
 * @author fanyu
 * @Date 2020/3/17
 * @version 1.00
 */
public class Aggregate21930 extends MongodbTestBase {
    private String clName;
    // 不能小于6
    private int num = 6;
    private List< Entity > list;

    @BeforeClass
    public void setUp() throws UnknownHostException {
        clName = springDBNameWithVersion + "_cl21930";
        list = new CopyOnWriteArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add(
                    new Entity( "a" + i, Entity.SEXS[ i % Entity.SEXS.length ],
                            i, i, Entity.COURSES ) );
        }
        mongoTemplate.insert( list, clName );
        mongoTemplate.indexOps( clName ).ensureIndex(
                new Index().named( "name" ).on( "name", Sort.Direction.ASC ) );
    }

    @Test
    public void test1() {
        Aggregation agg;
        AggregationResults< Entity > actResults;
        List< Entity > actList;
        // 带匹配符
        // 匹配到记录
        agg = Aggregation
                .newAggregation( match( Criteria.where( "age" ).lte( num ) ) );
        actResults = mongoTemplate.aggregate( agg, clName, Entity.class );
        actList = actResults.getMappedResults();
        Assert.assertEquals( actList, list );

        // 匹配不到记录
        agg = Aggregation
                .newAggregation( match( Criteria.where( "age" ).gt( num ) ) );
        actResults = mongoTemplate.aggregate( agg, clName, Entity.class );
        actList = actResults.getMappedResults();
        Assert.assertEquals( actList.size(), 0 );

        // （1）选择普通字段
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).lt( num ) ),
                project( "name", "grade", "courses" ) );
        actResults = mongoTemplate.aggregate( agg, clName, Entity.class );
        actList = actResults.getMappedResults();
        Assert.assertEquals( actList.size(), list.size() );
        for ( int i = 0; i < list.size(); i++ ) {
            Entity expInit = list.get( i );
            Entity exp = new Entity( expInit.getName(), null, 0,
                    expInit.getGrade(), expInit.getCourses() );
            exp.setId( expInit.getId() );
            Assert.assertEquals( actList.get( i ), exp );
        }

        // （2）选择_id字段
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).lt( num ) ), project( "_id" ) );
        actResults = mongoTemplate.aggregate( agg, clName, Entity.class );
        actList = actResults.getMappedResults();
        Assert.assertEquals( actList.size(), list.size() );
        for ( int i = 0; i < list.size(); i++ ) {
            Entity expInit = list.get( i );
            Entity exp = new Entity();
            exp.setId( expInit.getId() );
            Assert.assertEquals( actList.get( i ), exp );
        }

        // （3）(6)排除其它普通字段
        // 因为 Projections by the mongodb aggregation framework only support the
        // exclusion of the _id field!
        // 所以（3）测试点没有实现自动化

        // （4）排除_id字段 due to $projection requires at least one output field

        // （5）选择普通字段和排除_id
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).lt( num ) ),
                project( "name", "sex", "age", "grade", "courses" )
                        .andExclude( "_id" ) );
        actResults = mongoTemplate.aggregate( agg, clName, Entity.class );
        actList = actResults.getMappedResults();
        for ( int i = 0; i < list.size(); i++ ) {
            Entity act = actList.get( i );
            Entity exp = list.get( i );
            Assert.assertEquals( act, new Entity( exp.getName(), exp.getSex(),
                    exp.getAge(), exp.getGrade(), exp.getCourses() ) );
        }

        // （7）返回_id字段和选择的普通字段
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).lt( num ) ),
                project( "name", "sex", "age", "_id" ) );
        actResults = mongoTemplate.aggregate( agg, clName, Entity.class );
        actList = actResults.getMappedResults();
        for ( int i = 0; i < list.size(); i++ ) {
            Entity act = actList.get( i );
            Entity initexp = list.get( i );
            Entity exp = new Entity( initexp.getName(), initexp.getSex(),
                    initexp.getAge(), 0, null );
            exp.setId( initexp.getId() );
            Assert.assertEquals( act, exp );
        }

        // 带匹配符+选择符+sort+skip+limit
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).lt( num ) ),
                project( "name", "sex", "age", "grade", "courses", "_id" ),
                sort( Sort.Direction.ASC, "age" ), skip( 1 ), limit( num ) );
        actResults = mongoTemplate.aggregate( agg, clName, Entity.class );
        actList = actResults.getMappedResults();
        Assert.assertEquals( actList, list.subList( 1, num ) );

        // 带分组进行聚集
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).exists( true ) ), group( "sex" ),
                sort( Sort.Direction.DESC, "_id" ), skip( 1 ), limit( 2 ) );
        actResults = mongoTemplate.aggregate( agg, clName, Entity.class );
        actList = actResults.getMappedResults();
        Assert.assertEquals( actList.size(), 1 );
        Assert.assertEquals( actList.get( 0 ).getId(), "m" );
    }

    @Test
    public void test2() {
        Aggregation agg;
        AggregationResults< BasicDBObject > actResults;
        // 带匹配符、分组、选择符、聚集符进行聚集
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).exists( true ) ),
                group( "sex" ).avg( "$age" ).as( "avg_age" ),
                project( "avg_age" ), sort( Sort.Direction.ASC, "avg_age" ),
                skip( 1 ), limit( 2 ) );
        actResults = mongoTemplate.aggregate( agg, clName,
                BasicDBObject.class );
        int sum1 = 0;
        int count1 = 0;
        for ( Entity entity : list ) {
            if ( entity.getSex().equals( "w" ) ) {
                sum1 += entity.getAge();
                count1++;
            }
        }
        Assert.assertEquals( actResults,
                Collections.singletonList( new BasicDBObject( "_id", "w" )
                        .append( "avg_age", ( double ) sum1 / count1 ) ) );

        // exists group avg sum sort
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).exists( true ) ),
                group( "sex" ).sum( "$age" ).as( "sum_age" ),
                project( "_id", "sum_age" ),
                sort( Sort.Direction.ASC, "sum_age" ), limit( 2 ) );
        actResults = mongoTemplate.aggregate( agg, clName,
                BasicDBObject.class );
        double sum2 = 0;
        int total = 0;
        for ( Entity entity : list ) {
            total += entity.getAge();
            if ( entity.getSex().equals( "w" ) ) {
                sum2 += entity.getAge();
            }
        }
        Assert.assertEquals( actResults, Arrays.asList(
                new BasicDBObject( "_id", "m" ).append( "sum_age",
                        total - sum2 ),
                new BasicDBObject( "_id", "w" ).append( "sum_age", sum2 ) ) );

        // exists group max
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).exists( true ) ),
                group( "sex" ).max( "$age" ).as( "max_age" ),
                project( "_id", "max_age" ),
                sort( Sort.Direction.ASC, "max_age" ), limit( 2 ) );
        actResults = mongoTemplate.aggregate( agg, clName,
                BasicDBObject.class );
        Assert.assertEquals( actResults,
                Arrays.asList(
                        new BasicDBObject( "_id", "m" ).append( "max_age",
                                num - 2 ),
                        new BasicDBObject( "_id", "w" ).append( "max_age",
                                num - 1 ) ) );

        // exists group min
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).exists( true ) ),
                group( "sex" ).min( "$age" ).as( "min_age" ),
                project( "_id", "min_age" ),
                sort( Sort.Direction.ASC, "min_age" ), limit( 2 ) );
        actResults = mongoTemplate.aggregate( agg, clName,
                BasicDBObject.class );
        Assert.assertEquals( actResults, Arrays.asList(
                new BasicDBObject( "_id", "m" ).append( "min_age", 0 ),
                new BasicDBObject( "_id", "w" ).append( "min_age", 1 ) ) );

        // exists group addtoset
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).exists( true ) ),
                group( "sex" ).addToSet( "$age" ).as( "set_age" ),
                project( "set_age" ), sort( Sort.Direction.ASC, "set_age" ),
                limit( 2 ) );
        actResults = mongoTemplate.aggregate( agg, clName,
                BasicDBObject.class );
        Set< Integer > set1 = new HashSet<>();
        Set< Integer > set2 = new HashSet<>();
        for ( Entity entity : list ) {
            if ( entity.getSex().equals( "m" ) ) {
                set1.add( entity.getAge() );
            } else {
                set2.add( entity.getAge() );
            }
        }
        Assert.assertEquals( actResults.getMappedResults().size(), 2 );

        // exists group first
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).exists( true ) ),
                group( "sex" ).first( "$age" ).as( "first_age" ),
                project( "_id", "first_age" ),
                sort( Sort.Direction.ASC, "first_age" ), limit( 2 ) );
        actResults = mongoTemplate.aggregate( agg, clName,
                BasicDBObject.class );
        Assert.assertEquals( actResults, Arrays.asList(
                new BasicDBObject( "_id", "m" ).append( "first_age", 0 ),
                new BasicDBObject( "_id", "w" ).append( "first_age", 1 ) ) );

        // exists group last
        // TODO:SEQUOIADBMAINSTREAM-5656
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).exists( true ) ),
                group( "sex" ).last( "$age" ).as( "last_age" ),
                project( "_id", "last_age" ),
                sort( Sort.Direction.ASC, "last_age" ), limit( 2 ) );
        actResults = mongoTemplate.aggregate( agg, clName,
                BasicDBObject.class );
        // Assert.assertEquals( results7, Arrays.asList(
        // new BasicDBObject( "_id", "m" )
        // .append( "last_age", num - 2 ),
        // new BasicDBObject( "_id", "w" ).append( "last_age", num - 1 )
        // ) );

        // //lte group push
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).exists( true ) ),
                group( "sex" ).push( "$age" ).as( "push_age" ),
                project( "_id", "push_age" ),
                sort( Sort.Direction.ASC, "push_age" ), limit( 2 ) );
        actResults = mongoTemplate.aggregate( agg, clName,
                BasicDBObject.class );
        Assert.assertEquals( actResults, Arrays.asList(
                new BasicDBObject( "_id", "m" ).append( "push_age", set1 ),
                new BasicDBObject( "_id", "w" ).append( "push_age", set2 ) ) );
    }

    @Test
    public void test3() {
        Aggregation agg;
        AggregationResults< Entity > actResults;
        // 集合不存在数据，进行聚集
        String clName1 = clName + "-test3";
        if ( mongoTemplate.collectionExists( clName1 ) ) {
            mongoTemplate.dropCollection( clName1 );
        }
        mongoTemplate.createCollection( clName1 );
        agg = Aggregation
                .newAggregation( match( Criteria.where( "age" ).lte( num ) ) );
        actResults = mongoTemplate.aggregate( agg, clName1, Entity.class );
        Assert.assertEquals( actResults.getMappedResults().size(), 0 );
        mongoTemplate.dropCollection( clName1 );

        // 匹配字段不存在，进行聚集
        agg = Aggregation
                .newAggregation( match( Criteria.where( "age1" ).lte( num ) ) );
        AggregationResults< String > results2 = mongoTemplate.aggregate( agg,
                clName, String.class );
        Assert.assertEquals( results2.getMappedResults().size(), 0 );

        // 分组的字段不存在，进行聚集
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).lte( num ) ), group( "age1" ) );
        AggregationResults< BasicDBObject > results3 = mongoTemplate
                .aggregate( agg, clName, BasicDBObject.class );
        Assert.assertEquals( results3,
                Collections.singleton( new BasicDBObject( "_id", null ) ) );

        // 选择字段不存在，进行聚集
        // 选择的字段不存在会报错
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).lte( num ) ), group( "age" ),
                project( "age1" ) );
        try {
            mongoTemplate.aggregate( agg, clName, String.class );
            Assert.fail( "exp fail but act success" );
        } catch ( IllegalArgumentException e ) {
            if ( !e.getMessage().contains( "age1" ) ) {
                throw e;
            }
        }

        // 聚集字段不存在
        agg = Aggregation.newAggregation(
                match( Criteria.where( "age" ).lte( num ) ),
                group( "age" ).sum( "age1" ).as( "sum_age1" ),
                project( "age" ) );
        AggregationResults< BasicDBObject > results5 = mongoTemplate
                .aggregate( agg, clName, BasicDBObject.class );
        Assert.assertEquals( results5.getMappedResults().size(), num );
    }

    @Test
    private void test4() {
        // 参数校验
        Aggregation agg = Aggregation.newAggregation(
                match( Criteria.where( "$" ).lte( num ) ),
                group( "$age" ).sum( "$age1" ).as( "$sum_age1" ),
                project( "age" ) );
        try {
            mongoTemplate.aggregate( agg, clName, BasicDBObject.class );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( InvalidDataAccessApiUsageException e ) {
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
