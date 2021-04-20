package com.mongodb.springdata;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.regex.Pattern;

import org.bson.types.ObjectId;
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

import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-21927:find操作
 * @author fanyu
 * @Date:2020/3/19
 * @version:1.0
 */
public class Find21927 extends MongodbTestBase {
    private String clName = springDBNameWithVersion + "_cl21927";
    // 不能小于10
    private int num = 10;
    private List< Entity > list;

    @BeforeClass
    public void setUp() {
        list = new CopyOnWriteArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add(
                    new Entity( "a" + i, Entity.SEXS[ i % Entity.SEXS.length ],
                            i, i, Entity.COURSES ) );
        }
        mongoTemplate.insert( list, clName );
        mongoTemplate.indexOps( clName ).ensureIndex(
                new Index().named( "name" ).on( "name", Sort.Direction.ASC ) );
        mongoTemplate.indexOps( clName ).ensureIndex(
                new Index().named( "age" ).on( "age", Sort.Direction.ASC ) );
    }

    @Test
    public void test1() {
        List< Entity > actList;
        // 不带条件查询
        actList = mongoTemplate.findAll( Entity.class, clName );
        Collections.sort( actList, new Comparator< Entity >() {
            @Override
            public int compare( Entity o1, Entity o2 ) {
                return o1.getAge() - o2.getAge();
            }
        } );
        Assert.assertEquals( actList, list );

        // 带空条件进行查询
        actList = mongoTemplate.find( new Query(), Entity.class, clName );
        Collections.sort( actList, new Comparator< Entity >() {
            @Override
            public int compare( Entity o1, Entity o2 ) {
                return o1.getAge() - o2.getAge();
            }
        } );
        Assert.assertEquals( actList, list );
    }

    @Test
    public void test2() {
        Query query;
        List< Entity > actList;
        // 带匹配符进行查询
        // and ne lt gt 查询
        query = new Query( Criteria.where( "age" ).lt( num ).gte( -1 )
                .andOperator( Criteria.where( "name" ).ne( "k" ) ) );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list );

        // or lte gte 查询
        Criteria criteria = Criteria.where( "name" ).exists( true ).orOperator(
                Criteria.where( "age" ).lte( num / 3 ),
                Criteria.where( "age" ).gt( 2 * num / 3 ) );
        query = new Query( criteria );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        List< Entity > expList2 = new ArrayList<>(
                list.subList( 0, num / 3 + 1 ) );
        expList2.addAll( list.subList( 2 * num / 3 + 1, num ) );
        Assert.assertEquals( actList, expList2 );

        // et无相关操作符

        // not sequoiadb不支持，与mongodb的用法不一样
        // query = new Query( Criteria.where( "age" ).not().gt( 1 ) );
        // actList = mongoTemplate.find( query, Entity.class, clName );
        // Assert.assertEquals( actList, list.subList( 0, 2 ) );

        // $in、$exists查询
        query = new Query(
                Criteria.where( "sex" ).exists( true ).in( "m", "w" ) );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list );

        // $nin、$exists查询
        query = new Query(
                Criteria.where( "sex" ).exists( true ).nin( "m", "w" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList.size(), 0 );

        // $mode $all
        query = new Query( Criteria.where( "age" ).mod( 3, 0 ).and( "courses" )
                .all( Entity.COURSES[ 0 ], Entity.COURSES[ 1 ],
                        Entity.COURSES[ 2 ] ) );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        List< Entity > expList6 = new ArrayList<>();
        for ( Entity entity : list ) {
            if ( entity.getAge() % 3 == 0 ) {
                expList6.add( entity );
            }
        }
        Assert.assertEquals( actList, expList6 );

        // regex
        Pattern pattern = Pattern.compile( "a[0-3]" );
        query = new Query( Criteria.where( "name" ).regex( pattern ) );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list.subList( 0, 4 ) );

        // $size sequoiadb 不支持
        // query = new Query( Criteria.where( "courses" ).size( 3 ) );
        // List< Entity > list7 = mongoTemplate.find( query, Entity.class,
        // clName );
        // Assert.assertEquals( list7, list, "query = " + query.toString() );

        // $type sequoiadb 不支持
        // query = new Query( Criteria.where( "sex" ).type( 2 ) );
        // actList = mongoTemplate.find( query, Entity.class, clName );
        // Assert.assertEquals( actList, list, "query = " + query.toString() );
    }

    @Test
    public void test3() {
        Query query;
        List< Entity > actList;
        // 带匹配符+sort查询
        // 单个字段正序查询
        query = new Query( Criteria.where( "age" ).gte( 0 ).lt( num / 3 ) );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list.subList( 0, num / 3 ) );

        // 单个字段逆序查询
        query = new Query( Criteria.where( "age" ).gte( 0 ).lte( num / 3 ) );
        query.with( new Sort( Sort.Direction.DESC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        for ( int i = 0; i <= num / 3; i++ ) {
            Assert.assertEquals( actList.get( i ), list.get( num / 3 - i ) );
        }

        // 多个字段正逆序
        query = new Query( Criteria.where( "age" ).gte( 0 ).lte( num ) );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        query.with( new Sort( Sort.Direction.DESC, "nmae" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list );

        // 不存在的字段排序
        query = new Query().with( new Sort( Sort.Direction.ASC, "age1" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Collections.sort( actList, new Comparator< Entity >() {
            @Override
            public int compare( Entity o1, Entity o2 ) {
                return o1.getAge() - o2.getAge();
            }
        } );
        Assert.assertEquals( actList, list );
    }

    @Test
    public void test4() {
        Query query;
        List< Entity > actList;
        // 带匹配符、sort、limit、skip查询
        // skip大于等于匹配的记录数
        query = new Query( Criteria.where( "age" ).is( num / 3 ) );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        query.skip( num / 3 ).limit( num );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList.size(), 0 );

        // skip小于匹配数，limit小于匹配数
        query = new Query( Criteria.where( "age" ).lte( num ) );
        query.skip( 1 ).limit( 2 * num / 3 );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list.subList( 1, 2 * num / 3 + 1 ) );

        // skip小于匹配数，limit大于匹配数
        query = new Query( Criteria.where( "age" ).lte( 2 * num / 3 ) );
        query.skip( num / 3 ).limit( num );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList,
                list.subList( num / 3, 2 * num / 3 + 1 ) );

        // 匹配的记录数是0，skip limit
        query = new Query( Criteria.where( "age" ).gt( num ) );
        query.skip( num / 3 ).limit( num );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList.size(), 0 );
        // 带匹配符、sort、limit、skip、hint查询
        // spring data 不支持使用键值对的方式
        // 索引存在，hint使用索引名
        query = new Query( Criteria.where( "age" ).gt( -1 ).lt( num ) );
        query.skip( 1 ).limit( num ).withHint( "name" );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list.subList( 1, num ) );
        // 索引不存在，hint使用索引名
        query = new Query( Criteria.where( "age" ).gt( -1 ).lt( num ) );
        query.skip( 1 ).limit( num ).withHint( "name" + "-inexistences" );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list.subList( 1, num ) );
        // limit为0
        query = new Query( Criteria.where( "age" ).lte( num ) );
        query.limit( 0 );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list );
        // limit为-1
        query = new Query( Criteria.where( "age" ).lte( num ) );
        query.limit( -1 );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list );
    }

    @Test
    public void test5() {
        Query query;
        List< Entity > actList;
        // 带匹配符、sort、limit、skip、hint、选择符查询
        Sort sort = new Sort( Sort.Direction.ASC, "age" );
        query = new Query( Criteria.where( "age" ).gt( -1 ) );
        query.with( sort );
        // 存在的字段,匹配到记录，排除_id
        query.fields().exclude( "_id" );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList.size(), num );
        for ( Entity entity : actList ) {
            Assert.assertEquals( entity.getId(), null );
        }
        // 存在的字段,匹配不到记录，排除_id
        query = new Query( Criteria.where( "age" ).gt( num ) );
        query.fields().exclude( "_id" );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList.size(), 0 );

        // 排除其它字段
        query = new Query( Criteria.where( "age" ).lt( num / 3 ) );
        query.fields().exclude( "age" ).exclude( "sex" );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        for ( int i = 0; i < num / 3; i++ ) {
            Entity expInit = list.get( i );
            Entity exp = new Entity( expInit.getName(), null, 0,
                    expInit.getGrade(), expInit.getCourses() );
            exp.setId( expInit.getId() );
            Assert.assertEquals( actList.get( i ), exp );
        }

        // 选择_id字段
        query = new Query( Criteria.where( "age" ).lt( num / 3 ) );
        query.fields().include( "_id" );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        for ( int i = 0; i < num / 3; i++ ) {
            Entity exp = new Entity();
            exp.setId( list.get( i ).getId() );
            Assert.assertEquals( actList.get( i ), exp );
        }
        // 选择其它字段
        query = new Query( Criteria.where( "age" ).lt( num / 3 ) );
        query.fields().include( "name" );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        for ( int i = 0; i < num / 3; i++ ) {
            Entity exp = new Entity();
            exp.setName( list.get( i ).getName() );
            exp.setId( list.get( i ).getId() );
            Assert.assertEquals( actList.get( i ), exp );
        }

        // 选择其它字段和排除_id
        query = new Query( Criteria.where( "age" ).lt( num / 3 ) );
        query.fields().include( "name" ).exclude( "_id" );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        for ( int i = 0; i < num / 3; i++ ) {
            Entity exp = new Entity();
            exp.setName( list.get( i ).getName() );
            Assert.assertEquals( actList.get( i ), exp );
        }

        // 选择其它字段和选择_id
        query = new Query( Criteria.where( "age" ).lt( num / 3 ) );
        query.fields().include( "name" ).include( "_id" );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        for ( int i = 0; i < num / 3; i++ ) {
            Entity exp = new Entity();
            exp.setName( list.get( i ).getName() );
            exp.setId( list.get( i ).getId() );
            Assert.assertEquals( actList.get( i ), exp );
        }

        // 排除其它字段和排除_id
        query = new Query( Criteria.where( "age" ).lt( num / 3 ) );
        query.fields().exclude( "name" ).exclude( "_id" ).exclude( "sex" )
                .exclude( "age" ).exclude( "courses" );
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        for ( int i = 0; i < num / 3; i++ ) {
            Entity exp = new Entity();
            exp.setGrade( list.get( i ).getGrade() );
            Assert.assertEquals( actList.get( i ), exp );
        }

        // 排除选择其它字段和选择_id
        query = new Query( Criteria.where( "age" ).lt( num / 3 ) );
        query.fields().exclude( "name" ).include( "_id" ).exclude( "sex" )
                .exclude( "age" ).exclude( "courses" );
        try {
            mongoTemplate.find( query, Entity.class, clName );
            Assert.fail( "exp fail but act success!!!" );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "Invalid Argument" ) ) {
                throw e;
            }
        }

        // 选择符 eleMatch 在java 中进行测试

        // 选择符 slice
        query = new Query( Criteria.where( "age" ).gt( -1 ) );
        query.with( sort ).withHint( "name" ).skip( 2 );
        query.fields().slice( "courses", 3 );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list.subList( 2, num ) );

        // 匹配的记录数为0条
        query = new Query( Criteria.where( "age" ).gt( num ) );
        query.with( sort ).withHint( "name" ).skip( 2 );
        query.fields().slice( "courses", 3 );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList.size(), 0 );

        // 选择的字段不存在
        query = new Query( Criteria.where( "age" ).lt( num ) );
        query.with( sort ).withHint( "name" ).skip( 2 );
        query.fields().include( "k" );
        actList = mongoTemplate.find( query, Entity.class, clName );
        for ( Entity entity : actList ) {
            Assert.assertNotNull( entity.getId() );
        }

        // 排除的字段不存在
        query = new Query( Criteria.where( "age" ).lt( num ) );
        query.with( sort ).withHint( "name" ).skip( 2 );
        query.fields().exclude( "k" );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList, list.subList( 2, num ) );

        // 选择字段部分字段存在，部分字段不存在
        query = new Query( Criteria.where( "age" ).lt( num ) );
        query.with( sort ).withHint( "name" ).skip( 2 );
        query.fields().include( "k" ).include( "name" );
        actList = mongoTemplate.find( query, Entity.class, clName );
        List< Entity > expList19 = list.subList( 2, num );
        for ( int i = 0; i < expList19.size(); i++ ) {
            Entity act = actList.get( i );
            Entity exp = new Entity();
            exp.setName( expList19.get( i ).getName() );
            exp.setId( expList19.get( i ).getId() );
            Assert.assertEquals( act, exp );
        }

        // 排除字段部分字段存在，部分字段不存在
        query = new Query( Criteria.where( "age" ).lt( num ) );
        query.with( sort ).withHint( "name" ).skip( 2 );
        query.fields().exclude( "k" ).exclude( "name" );
        actList = mongoTemplate.find( query, Entity.class, clName );
        List< Entity > expList20 = list.subList( 2, num );
        for ( int i = 0; i < expList20.size(); i++ ) {
            Entity exp = new Entity();
            Entity initExp = expList20.get( i );
            exp.setAge( initExp.getAge() );
            exp.setCourses( initExp.getCourses() );
            exp.setGrade( initExp.getGrade() );
            exp.setId( initExp.getId() );
            exp.setSex( initExp.getSex() );
            Entity act = actList.get( i );
            Assert.assertEquals( act, exp );
        }

        // 排除_id和选择其他字段
        query = new Query( Criteria.where( "age" ).gt( -1 ) );
        query.with( sort ).withHint( "name" );
        query.fields().exclude( "_id" ).include( "name" ).include( "sex" )
                .include( "age" ).include( "courses" );
        actList = mongoTemplate.find( query, Entity.class, clName );
        for ( Entity entity : actList ) {
            Assert.assertEquals( entity.getId(), null );
        }
    }

    @Test
    public void test6() {
        Query query;
        List< Entity > actList;
        // 带不存在的字段进行查询
        query = new Query( Criteria.where( "k" ).is( "1" ) );
        actList = mongoTemplate.find( query, Entity.class, clName );
        Assert.assertEquals( actList.size(), 0 );

        // 集合不存在数据，进行查询
        mongoTemplate.createCollection( clName + "_empty" );
        query = new Query( Criteria.where( "age" ).is( 0 ) );
        actList = mongoTemplate.find( query, Entity.class, clName + "_empty" );
        Assert.assertEquals( actList.size(), 0 );
        mongoTemplate.dropCollection( clName + "_empty" );
    }

    @Test
    public void test7() {
        Query query;
        // 查询单个
        query = new Query( Criteria.where( "age" ).gt( -1 ) );
        query.withHint( "name" ).with( new Sort( Sort.Direction.ASC, "age" ) );
        query.fields().include( "name" ).include( "sex" ).include( "age" )
                .include( "courses" );
        Entity entity1 = mongoTemplate.findOne( query, Entity.class, clName );
        Assert.assertEquals( entity1, list.get( 0 ) );

        // 使用id进行查询
        Entity entity2 = mongoTemplate.findById(
                new ObjectId( list.get( 0 ).getId() ), Entity.class, clName );
        Assert.assertEquals( entity2, list.get( 0 ) );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }
}
