package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.client.MongoDatabase;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21933:增删改查数据库
 * @author fanyu
 * @Date 2020/3/23
 * @version 1.00
 */
public class CrudDB21933 extends MongodbTestBase {
    private MongoDatabase db;
    private String dbName = javaDBNameWithVersion + "_db21933";
    private String clName = javaDBNameWithVersion + "_cl21933";

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = client.getDatabase( dbName );
    }

    @Test
    public void test() {
        // 数据库不存在，删除数据库
        db.drop();
        client.dropDatabase( dbName );

        // 当数据库下有集合时，会自动创建数据库
        db.createCollection( clName );

        // 数据库存在时，获取数据库
        db = client.getDatabase( dbName );
        Assert.assertNotNull( db );

        // 列取数据库
        // 存在多个列取数据库,可能存在其他数据库，只能简单比较；
        // 数据库存在零个数据库手，获取数据库，手工进行验证
        List< String > list = client.listDatabaseNames()
                .into( new ArrayList< String >() );
        Assert.assertTrue( list.contains( dbName ) );

        // 数据库存在，删除数据库
        db.drop();
        Assert.assertTrue( !client.listDatabaseNames()
                .into( new ArrayList< String >() ).contains( dbName ) );
    }

    @AfterClass
    public void tearDown( ITestContext context ) {
    }
}
