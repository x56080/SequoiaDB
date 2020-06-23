package com.sequoiadb.crud;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: Sdv6606.java test content: 对date和timestamp类型的数据进行排序
 * (对应问题的：SEQUOIADBMAINSTREAM-1134) testlink case: seqDB-6606
 * 
 * @author zengxianquan
 * @date 2016年12月21日
 * @version 1.00
 */
public class Sdv6606 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "cl6606";
    private List< BSONObject > resList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            sdb.disconnect();
        }
    }

    @Test
    public void testSort() {
        DBCollection cl = null;
        try {
            cl = cs.createCollection( clName );
            BSONObject indexBson = new BasicBSONObject();
            indexBson.put( "a", 1 );
            cl.createIndex( "Idx", indexBson, false, false );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        insertData( cl );
        // 初始化resList属性成员
        initResList();
        // 检验数据
        checkSort( cl );
    }

    public void checkSort( DBCollection cl ) {
        DBCursor cursor = null;
        try {
            BSONObject orderBy = new BasicBSONObject();
            orderBy.put( "a", 1 );
            cursor = cl.query( null, null, orderBy, null );
            BSONObject res = null;
            int i = 0;
            while ( cursor.hasNext() ) {
                res = cursor.getNext();
                res.removeField( "_id" );
                if ( !res.equals( resList.get( i ) ) ) {
                    Assert.fail( "check data is error" );
                }
                i++;
            }
            // 检验数据的完整性
            if ( i != resList.size() ) {
                Assert.fail( "check data is error" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
    }

    public void insertData( DBCollection cl ) {
        try {
            cl.insert(
                    "{a : { '$timestamp' : '2001-04-03-10.11.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2001-04-03-10.12.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2001-04-03-10.11.13.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2001-04-03-11.11.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2001-04-03-10.11.12.123457' }}" );

            cl.insert(
                    "{a : { '$timestamp' : '2002-04-03-10.11.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2002-04-03-10.12.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2002-04-03-10.11.13.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2002-04-03-11.11.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2002-04-03-10.11.12.123457' }}" );

            cl.insert(
                    "{a : { '$timestamp' : '2002-04-02-10.11.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2002-04-02-10.12.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2002-04-02-10.11.13.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2002-04-02-11.11.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2002-04-02-10.11.12.123457' }}" );

            cl.insert(
                    "{a : { '$timestamp' : '2000-04-04-10.11.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2000-04-04-10.12.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2000-04-04-10.11.13.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2000-04-04-11.11.12.123456' }}" );
            cl.insert(
                    "{a : { '$timestamp' : '2000-04-04-10.11.12.123457' }}" );

            cl.insert( "{a:{'$date':'2001-04-03'}}" );
            cl.insert( "{a:{'$date':'2001-04-02'}}" );
            cl.insert( "{a:{'$date':'2001-05-03'}}" );
            cl.insert( "{a:{'$date':'2000-04-03'}}" );
            cl.insert( "{a:{'$date':'2000-04-04'}}" );
            cl.insert( "{a:{'$date':'2002-04-03'}}" );
            cl.insert( "{a:{'$date':'2002-04-04'}}" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public void initResList() {
        BSONObject bson;
        bson = ( BSONObject ) JSON.parse( "{a:{'$date': '2000-04-03'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON.parse( "{'a': {'$date': '2000-04-04'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2000-04-04-10.11.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2000-04-04-10.11.12.123457'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2000-04-04-10.11.13.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2000-04-04-10.12.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2000-04-04-11.11.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON.parse( "{'a': {'$date': '2001-04-02'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON.parse( "{'a': {'$date': '2001-04-03'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2001-04-03-10.11.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2001-04-03-10.11.12.123457'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2001-04-03-10.11.13.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2001-04-03-10.12.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2001-04-03-11.11.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON.parse( "{'a': {'$date': '2001-05-03'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-02-10.11.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-02-10.11.12.123457'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-02-10.11.13.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-02-10.12.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-02-11.11.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON.parse( "{'a': {'$date': '2002-04-03'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-03-10.11.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-03-10.11.12.123457'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-03-10.11.13.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-03-10.12.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON
                .parse( "{'a': {'$timestamp': '2002-04-03-11.11.12.123456'}}" );
        resList.add( bson );
        bson = ( BSONObject ) JSON.parse( "{'a': {'$date': '2002-04-04'}}" );
        resList.add( bson );
    }
}
