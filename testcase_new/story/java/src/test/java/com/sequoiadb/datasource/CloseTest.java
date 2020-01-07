package com.sequoiadb.datasource;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Parameters;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.SequoiadbDatasource;
import com.sequoiadb.base.SequoiadbOption;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ServerAddress;

public class CloseTest extends DataSourceTestBase {
    private SequoiadbDatasource datasource = null;
    // private SequoiadbDatasource ds1 = null;

    @BeforeClass
    public void initEnv() {
        boolean retVal = super.init();
        Assert.assertTrue( retVal );
        try {
            getAddrList();
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @BeforeMethod
    public void createDataSource() {
        try {
            if ( datasource == null ) {
                datasource = new SequoiadbDatasource( this.coordAddr,
                        this.userName, this.password, null );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @AfterMethod
    public void closeDataSource() {
        try {
            if ( null != datasource ) {
                datasource.close();
                datasource = null;
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    /**
     * 关闭连接池
     */
    @Test
    public void closeAfterGetConn() {
        Sequoiadb sdb = null;
        boolean isGetConn = false;
        try {
            sdb = datasource.getConnection();
            datasource.close();
            Assert.assertEquals( sdb.isValid(), false );
            Assert.assertEquals( datasource.getIdleConnNum(), 0 );
            Assert.assertEquals( datasource.getUsedConnNum(), 0 );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

        try {
            if ( null != datasource ) {
                datasource.getConnection();
                Assert.assertTrue( false );
            }
        } catch ( InterruptedException e ) {
            Assert.assertFalse( true, e.getMessage() );
        } catch ( BaseException e ) {
            // TODO: handle exception
            judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }

    }

    /**
     * 关闭禁用的连接池
     */
    @Test
    public void closeAfterDisabled() {
        Sequoiadb sdb = null;
        Sequoiadb sdb1 = null;
        boolean isGetConn = false;
        boolean isDisabled = false;
        try {
            sdb1 = datasource.getConnection();
            datasource.disableDatasource();
            sdb = datasource.getConnection();
            datasource.close();
            Assert.assertEquals( sdb.isValid(), true );
            Assert.assertEquals( sdb1.isValid(), false );
        } catch ( InterruptedException e ) {
            Assert.assertFalse( true, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

        try {
            datasource.getConnection();
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            // TODO: handle exception
            judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }

        try {
            datasource.releaseConnection( sdb );
            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }

    /**
     * 重复关闭连接池
     */
    @Test
    public void closeAfterClose() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            datasource.close();
            datasource.close();
        } catch ( BaseException e ) {
            // TODO: handle exception
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        }
    }

}
