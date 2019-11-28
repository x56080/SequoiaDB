package com.sequoiadb.ssl;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.basicoperation.Commlib;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbConfTestBase;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestSSL11314 test interface:setUseSSL(boolean useSSL)/getUseSSL()
 * operating steps: 1.sequoaidb configuration must open SSL connect 2.call
 * setUseSSL(boolean useSSL) to set SSL to true 3.create cs/cl，insert data
 * 4.intercepted coord port message，view the message transfer to
 * ciphertext（manual test） 5.test getUseSSL()
 * 
 * @author wuyan
 * @Date 2017.4.7
 * @version 1.00
 */
public class TestSSL11314 extends SdbConfTestBase {
    private String clName = "cl_11314";
    private static Sequoiadb sdb = null;
    private static Sequoiadb db = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @Override
    protected void setNodeConf() {
        coordConf.put( "usessl", true );
    }

    @BeforeClass
    public void setUp() {
        System.out.println( this.getClass().getName() + " begin at "
                + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss:S" )
                        .format( new Date() ) );
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        if ( Commlib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @Test
    public void testSSL() {
        try {
            // not open ssl
            ConfigOptions options = new ConfigOptions();
            Assert.assertEquals( options.getUseSSL(), false, "not open ssl" );

            // open ssl
            options.setUseSSL( true );
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "", options );

            // test createcl
            cs = db.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName );

            // test insert
            String value = "{a:1}";
            cl.insert( value );
            Assert.assertEquals( 1, cl.getCount( value ), "insert data error" );

            // test getSSL()
            boolean flag = options.getUseSSL();
            Assert.assertEquals( flag, true, "ssl should open" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getErrorCode() + e.getMessage() );
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            System.out.println( this.getClass().getName() + " end at "
                    + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss:S" )
                            .format( new Date() ) );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
            db.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

}
