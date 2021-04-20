package com.mongodb.java;

import java.net.UnknownHostException;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.utils.MongodbTestBase;

/**
 * @Description seqDB-21933:增删改查数据库
 * @author fanyu
 * @Date 2020/3/17
 * @version 1.00
 */
public class CrudDB21933 extends MongodbTestBase {
    private DB db;
    private String dbName = javaDBNameWithVersion + "_db21933";
    private String clName = javaDBNameWithVersion + "_cl21933";

    @BeforeClass
    public void setUp() throws UnknownHostException {
        db = client.getDB( dbName );
    }

    @Test
    public void test() {
        // 数据库不存在，删除数据库
        db.dropDatabase();

        // 当数据库下有数据时，会自动创建数据库
        db.createCollection( clName, new BasicDBObject() );

        // 数据库存在时，获取数据库
        db = client.getDB( dbName );
        Assert.assertNotNull( db );

        // 列取数据库
        // 存在多个列取数据库,可能存在其他数据库，只能简单比较；
        // 数据库存在零个数据库手，获取数据库，手工进行验证
        List< String > list = client.getDatabaseNames();
        Assert.assertTrue( list.contains( dbName ) );

        // 数据库存在，删除数据库
        db.dropDatabase();
        List< String > list1 = client.getDatabaseNames();
        Assert.assertFalse( list1.contains( dbName ) );
    }

    @AfterClass
    public void tearDown() {
    }
}
