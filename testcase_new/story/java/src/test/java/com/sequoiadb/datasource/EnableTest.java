package com.sequoiadb.datasource;

import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Parameters;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.SequoiadbDatasource;
import com.sequoiadb.base.SequoiadbOption;
import com.sequoiadb.exception.BaseException;

public class EnableTest extends DataSourceTestBase {

    @BeforeClass
    public void initEnv() {
        boolean retVal = super.init();
        Assert.assertTrue( retVal );
    }

    /**
     * 启用创建的连接池
     */
    @Test
    public void EnableAfterCreated() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            datasource.enableDatasource();
        } catch ( BaseException e ) {
            // TODO: handle exception
            Assert.assertFalse( true, e.getMessage() );
        } finally {
            datasource.close();
        }
    }

    /**
     * 启用禁用的连接池
     */
    @Test(invocationCount = 10, threadPoolSize = 5)
    public void EnableAfterDisabled() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            Sequoiadb sdb;
            sdb = datasource.getConnection();
            datasource.disableDatasource();
            datasource.enableDatasource();
            Assert.assertEquals( datasource.getUsedConnNum(), 1 );
            Sequoiadb sdb1 = datasource.getConnection();
            Assert.assertEquals( datasource.getUsedConnNum(), 2 );
            Thread.sleep( 10 );
            // Assert.assertTrue(datasource.getIdleConnNum() >= 0);
            datasource.releaseConnection( sdb );
            Assert.assertEquals( datasource.getUsedConnNum(), 1 );
            datasource.releaseConnection( sdb1 );
            Assert.assertEquals( datasource.getUsedConnNum(), 0 );

        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } finally {
            datasource.close();
        }
    }

    @Test
    public void EnableAfterDisabledByOption() {
        SequoiadbDatasource datasource = null;
        try {
            SequoiadbOption option = new SequoiadbOption();
            option.setMaxCount( 0 );
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, option );
            datasource.enableDatasource();

            Sequoiadb sdb = datasource.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } finally {
            datasource.close();
        }
    }

    /**
     * 启用关闭的连接池
     */
    @Test
    public void EnableAfterClosed() {
        SequoiadbDatasource datasource = null;
        try {
            datasource = new SequoiadbDatasource( this.coordAddr, userName,
                    password, null );
            datasource.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() );
        }
        try {
            datasource.enableDatasource();
        } catch ( BaseException e ) {
            // TODO: handle exception
            judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }
}
