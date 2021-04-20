package com.mongodb.springdata;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.springframework.data.domain.Sort;
import org.springframework.data.mongodb.UncategorizedMongoDbException;
import org.springframework.data.mongodb.core.IndexOperations;
import org.springframework.data.mongodb.core.index.Index;
import org.springframework.data.mongodb.core.index.IndexInfo;
import org.springframework.data.mongodb.core.query.Criteria;
import org.springframework.data.mongodb.core.query.Query;
import org.springframework.data.mongodb.core.query.Update;
import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DBObject;
import com.mongodb.WriteResult;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description: seqDB-21925:增删改查索引
 * @author fanyu
 * @Date:2020/3/12
 * @version:1.0
 */
public class CrudIndex21925 extends MongodbTestBase {
    private String clName = springDBNameWithVersion + "_cl21925";
    private IndexOperations indexOperations;

    @BeforeClass
    public void setUp() {
        indexOperations = mongoTemplate.indexOps( clName );
    }

    @Test
    public void test() {
        // 创建集合
        mongoTemplate.createCollection( clName );
        // 用户未创建索引，列取索引，默认存在$id索引
        List< IndexInfo > list = indexOperations.getIndexInfo();
        Assert.assertEquals( list.size(), 1, list.toString() );
        Assert.assertEquals( list.get( 0 ).getName(), "$id" );

        String[] indexNames1 = { "ascIndex21925", "compoundIndex21925",
                "descIndex21925" };
        String[] indexNames2 = { "$id", "ascIndex21925", "b_1", "c_-1",
                "compoundIndex21925", "descIndex21925", "f_-1_g_1" };

        // 指定索引名，单个字段，创建强制唯一升序索引
        indexOperations.ensureIndex( new Index().named( indexNames1[ 0 ] )
                .unique().on( "a", Sort.Direction.ASC ) );

        // 不指定索引名，单个字段，创建普通升序索引
        indexOperations
                .ensureIndex( new Index().on( "b", Sort.Direction.ASC ) );

        // 不指定索引名，单个字段，创建强制唯一降序索引
        indexOperations.ensureIndex(
                new Index().on( "c", Sort.Direction.DESC ).unique() );

        // 指定索引名，单个字段，创建普通降序索引
        indexOperations.ensureIndex( new Index().named( indexNames1[ 1 ] )
                .on( "d", Sort.Direction.DESC ).unique() );

        // 指定索引名，多个字段，创建强制唯一复合索引
        indexOperations.ensureIndex( new Index().named( indexNames1[ 2 ] )
                .on( "d", Sort.Direction.ASC ).on( "e", Sort.Direction.DESC )
                .unique() );

        // 不指定索引名，多个字段，创建普通复合索引
        indexOperations.ensureIndex( new Index().on( "f", Sort.Direction.DESC )
                .on( "g", Sort.Direction.ASC ) );

        // 重复创建索引，不会抛异常
        indexOperations.ensureIndex( new Index().named( indexNames1[ 0 ] )
                .unique().on( "a", Sort.Direction.ASC ) );

        // 列取索引
        List< IndexInfo > actIndexes = indexOperations.getIndexInfo();
        Assert.assertEquals( actIndexes.size(), indexNames2.length );
        for ( int i = 0; i < actIndexes.size(); i++ ) {
            Assert.assertEquals( actIndexes.get( i ).getName(),
                    indexNames2[ i ] );
        }

        // 做简单的操作
        crud();

        // 删除索引
        for ( int i = 1; i < indexNames2.length; i++ ) {
            indexOperations.dropIndex( indexNames2[ i ] );
        }
        Assert.assertEquals( indexOperations.getIndexInfo().size(), 1 );
    }

    private void crud() {
        List< DBObject > list = new ArrayList<>();
        int num = 10;
        for ( int i = 0; i < num; i++ ) {
            list.add( new BasicDBObject( "a", i ).append( "b", i + 1 )
                    .append( "c", i + 2 ).append( "d", i + 3 )
                    .append( "e", i + 4 ).append( "f", i + 5 )
                    .append( "g", i + 6 ) );
        }
        // 插入 无重复数据
        mongoTemplate.insert( list, clName );
        // 插入 有重复数据，索引键值重复
        for ( BSONObject bson : list ) {
            bson.removeField( "_id" );
        }
        try {
            mongoTemplate.insert( list, clName );
            Assert.fail( "exp fail but act success" );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }
        // 查询
        Query query1 = new Query();
        Sort sort = new Sort( Sort.Direction.ASC, "age" );
        query1.with( sort );

        List< BasicDBObject > result2 = mongoTemplate.find( query1,
                BasicDBObject.class, clName );
        for ( int i = 0; i < result2.size(); i++ ) {
            DBObject act = result2.get( i );
            DBObject exp = list.get( i );
            act.removeField( "_id" );
            act.removeField( "_class" );
            exp.removeField( "_id" );
            Assert.assertEquals( act, exp );
        }

        // 更新 无重复数据
        Query query3 = new Query( Criteria.where( "a" ).is( 1 ) );
        Update update3 = new Update().set( "a", num + 1 );
        WriteResult result3 = mongoTemplate.updateMulti( query3, update3,
                BasicDBObject.class, clName );
        Assert.assertEquals( result3.getN(), 1 );
        Assert.assertEquals( mongoTemplate.count(
                new Query( Criteria.where( "a" ).is( num + 1 ) ), clName ), 1 );

        // update 有重复数据：索引键值重复
        Query query4 = new Query( Criteria.where( "a" ).is( num / 2 ) );
        Update update4 = new Update().set( "a", num - 2 );
        try {
            mongoTemplate.updateMulti( query4, update4, BasicDBObject.class,
                    clName );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }

        // upsert 有重复数据：索引键值重复
        Query query5 = new Query( Criteria.where( "a" ).is( num / 2 ) );
        Update update5 = new Update().set( "a", num - 2 );
        try {
            mongoTemplate.upsert( query5, update5, BasicDBObject.class,
                    clName );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( UncategorizedMongoDbException e ) {
            if ( !e.getMessage().contains( "-38" ) ) {
                throw e;
            }
        }
        // 刪除
        mongoTemplate.remove( new Query(), clName );
        Assert.assertEquals( mongoTemplate.count( new Query(), clName ), 0 );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        dropCLByTestResult( context, this.toString(), mongoTemplate, clName );
    }
}
