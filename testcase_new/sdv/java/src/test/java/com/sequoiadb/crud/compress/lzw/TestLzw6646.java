package com.sequoiadb.crud.compress.lzw;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.compress.snappy.SnappyUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-6646:修改CL的压缩类型为snappy 1、CL压缩类型为lzw，修改CL的压缩类型为snappy 2、检查返回结果
 * @Author linsuqiang
 * @Date 2016-12-27
 * @Version 1.00
 */
public class TestLzw6646 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_6646";
    private SimpleDateFormat sdf = new SimpleDateFormat(
            "yyyy-MM-dd HH:mm:ss.S" );

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( SnappyUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            DBCollection cl = cs.createCollection( clName, ( BSONObject ) JSON
                    .parse( "{Compressed: true, CompressionType: 'lzw'}" ) );
            try {
                cl.alterCollection( ( BSONObject ) JSON
                        .parse( "{CompressionType: 'snappy'}" ) );
                throw new BaseException( -10000,
                        "Parameter 'CompressionType' shouldn't been altered successfully" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -6, e.getMessage() );
            }
            try {
                cl.alterCollection( ( BSONObject ) JSON.parse(
                        "{Compressed: true, CompressionType: 'snappy'}" ) );
                throw new BaseException( -10000,
                        "Parameter 'CompressionType' shouldn't been altered successfully" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -32, e.getMessage() );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }
}