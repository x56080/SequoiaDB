package com.mongodb.springdata;

import java.util.ArrayList;
import java.util.List;

import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.UncategorizedMongoDbException;
import org.springframework.data.mongodb.core.index.Index;
import org.springframework.data.mongodb.core.query.Query;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.utils.Entity;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21926:insert操作
 * @author fanyu
 * @Date 2020/3/20
 * @version 1.00
 */
public class Insert21926 extends MongodbTestBase {
    private String[] clNames = new String[ 2 ];

    @BeforeClass
    public void setUp() {
        clNames[ 0 ] = springDBNameWithVersion + "_cl21926A";
        clNames[ 1 ] = springDBNameWithVersion + "_cl21926B";
    }

    @Test
    public void test1() {
        String clName = clNames[ 0 ];
        // 创建普通索引
        mongoTemplate.indexOps( clName ).ensureIndex(
                new Index().named( "nmae" ).on( "name", Sort.Direction.ASC ) );
        // 准备记录
        int num = 5;
        List< Entity > list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add(
                    new Entity( "a" + i, Entity.SEXS[ i % Entity.SEXS.length ],
                            i, i, Entity.COURSES ) );
        }
        // 插入单条数据
        mongoTemplate.insert( list.get( 0 ), clName );
        // 重新设置"_id"字段，因为_id是唯一索引
        list.get( 0 ).setId( null );
        // 重复插入单条数据
        mongoTemplate.insert( list.get( 0 ), clName );
        // 插入多条数据：存在重复数据和不重复数据
        // 重新设置"_id"字段，因为_id是唯一索引
        list.get( 0 ).setId( null );
        mongoTemplate.insert( list, clName );
        // 检查结果
        Assert.assertEquals( mongoTemplate.count( new Query(), clName ),
                num + 2 );
        Query query = new Query();
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        List< Entity > actList = mongoTemplate.find( query, Entity.class,
                clName );
        for ( int i = 0; i < num + 2; i++ ) {
            if ( i < 3 ) {
                Entity act = actList.get( 0 );
                Entity exp = list.get( 0 );
                act.setId( null );
                exp.setId( null );
                Assert.assertEquals( act, exp,
                        "act = " + act + ",exp = " + exp );
            } else {
                Assert.assertEquals( actList.get( i ), list.get( i - 2 ) );
            }
        }
    }

    @Test
    public void test2() {
        String clName = clNames[ 1 ];
        // 创建强制唯一索引
        mongoTemplate.indexOps( clName ).ensureIndex( new Index()
                .named( "nmae" ).on( "name", Sort.Direction.ASC ).unique() );
        // 准备记录
        int num = 10;
        List< Entity > list = new ArrayList<>();
        for ( int i = 0; i < num; i++ ) {
            list.add(
                    new Entity( "a" + i, Entity.SEXS[ i % Entity.SEXS.length ],
                            i, i, Entity.COURSES ) );
        }
        // 插入单条数据
        mongoTemplate.insert( list.get( num / 2 ), clName );
        // 重新设置"_id"字段，因为_id是唯一索引
        list.get( num / 2 ).setId( "0" );
        // 重复插入单条数据
        try {
            mongoTemplate.insert( list.get( num / 2 ), clName );
            Assert.fail( "exp fail but act success" );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }
        // 插入多条数据：存在重复数据
        try {
            mongoTemplate.insert( list, clName );
            Assert.fail( "exp fail but act success" );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }
        // 检查结果
        Assert.assertEquals( mongoTemplate.count( new Query(), clName ),
                num / 2 + 1 );

        // 插入多条数据：不存在重复数据
        mongoTemplate.insert( list.subList( num / 2 + 1, num ), clName );
        Assert.assertEquals( mongoTemplate.count( new Query(), clName ), num );
        Query query = new Query();
        query.with( new Sort( Sort.Direction.ASC, "age" ) );
        List< Entity > actList = mongoTemplate.find( query, Entity.class,
                clName );
        for ( int i = 0; i < actList.size(); i++ ) {
            Entity act = actList.get( i );
            Entity exp = list.get( i );
            act.setId( null );
            exp.setId( null );
            Assert.assertEquals( act, exp, "act = " + act + ",exp = " + exp );
        }
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clNames );
    }
}
