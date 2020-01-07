package com.sequoiadb.datasource;

import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Parameters;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;

public class DisableTest extends DataSourceTestBase {
    @BeforeClass
    public void initEnv() {
        boolean retVal = super.init();
        Assert.assertTrue( retVal );
    }

    /**
     * 禁用连接池
     */
    @Test
    public void disableAfterGetConn() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            Sequoiadb sdb = datasource.getConnection();
            datasource.disableDatasource();
            Assert.assertEquals( datasource.getIdleConnNum(), 0 );
            Assert.assertEquals( datasource.getUsedConnNum(), 1 );
            datasource.releaseConnection( sdb );
            Assert.assertEquals( sdb.isValid(), false );
            Assert.assertEquals( datasource.getUsedConnNum(), 0 );
            Assert.assertEquals( datasource.getIdleConnNum(), 0 );
        } catch ( InterruptedException e ) {
            Assert.assertFalse( true, e.getMessage() );
        } catch ( BaseException e ) {
            // TODO: handle exception
            Assert.assertFalse( true, e.getMessage() );
        } finally {
            datasource.close();
        }
    }

    /**
     * 重复禁用连接池
     */
    @Test
    public void disableAfterDisable() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            datasource.disableDatasource();
            datasource.disableDatasource();
        } catch ( BaseException e ) {
            // TODO: handle exception
            Assert.assertFalse( true, e.getMessage() );
        } finally {
            datasource.close();
        }
    }

    /**
     * 禁用已关闭的连接池
     */
    @Test
    public void disableAfterClosed() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            datasource.close();
        } catch ( BaseException e ) {
            // TODO: handle exception
            Assert.assertFalse( true, e.getMessage() );
        }

        try {
            datasource.disableDatasource();
        } catch ( BaseException e ) {
            judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }
}
