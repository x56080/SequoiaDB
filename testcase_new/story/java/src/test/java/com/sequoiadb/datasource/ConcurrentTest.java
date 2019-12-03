package com.sequoiadb.datasource;

import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Random;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Parameters;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.SequoiadbDatasource;
import com.sequoiadb.base.SequoiadbOption;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ConfigOptions;

public class ConcurrentTest extends DataSourceTestBase {
    private SequoiadbDatasource ds1 = null;
    private SequoiadbDatasource ds2 = null;
    private SequoiadbDatasource ds3 = null;
    private SequoiadbDatasource ds4 = null;
    private SequoiadbDatasource ds5 = null;
    private SequoiadbDatasource ds6 = null;
    private SequoiadbDatasource ds7 = null;
    private SequoiadbDatasource ds8 = null;
    private SequoiadbDatasource ds9 = null;
    private DatasourceOptions option = null;
    private Random random = new Random();

    @BeforeClass
    public void createDatasource() {
        try {
            super.init();
            ds1 = new SequoiadbDatasource( this.coordAddr, userName, password,
                    null );
            ds2 = new SequoiadbDatasource( this.coordAddr, userName, password,
                    null );
            ds3 = new SequoiadbDatasource( this.coordAddr, userName, password,
                    null );
            ds4 = new SequoiadbDatasource( this.coordAddr, userName, password,
                    null );
            ds5 = new SequoiadbDatasource( this.coordAddr, userName, password,
                    null );
            ds6 = new SequoiadbDatasource( this.coordAddr, userName, password,
                    null );
            ds7 = new SequoiadbDatasource( this.coordAddr, userName, password,
                    null );
            ds8 = new SequoiadbDatasource( this.coordAddr, userName, password,
                    null );
            ds9 = new SequoiadbDatasource( this.coordAddr, userName, password,
                    null );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @AfterClass
    public void closeDatasource() {
        try {
            if ( null != sdbpools ) {
                sdbpools.close();
            }
            if ( ds1 != null ) {
                ds1.close();
            }

            if ( ds2 != null ) {
                ds2.close();
            }

            if ( ds3 != null ) {
                ds3.close();
            }

            if ( ds4 != null ) {
                ds4.close();
            }

            if ( ds5 != null ) {
                ds5.close();
            }

            if ( ds6 != null ) {
                ds5.close();
            }

            if ( ds7 != null ) {
                ds7.close();
            }

            if ( ds8 != null ) {
                ds8.close();
            }

            if ( ds9 != null ) {
                ds8.close();
            }

            if ( datasource_concurrent != null ) {
                datasource_concurrent.close();
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

    }

    /**
     * 并发申请释放连接
     */
    @Test(invocationCount = 50, threadPoolSize = 10)
    void getConnection() {
        try {
            if ( ds1 != null ) {
                Sequoiadb sdb = ds1.getConnection();
                Assert.assertEquals( sdb.isValid(), true );
                Thread.sleep( random.nextInt( 10 ) );
                ds1.releaseConnection( sdb );
            }
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() );
        }
    }

    /**
     * 增加或者删除coordAddr并发
     */
    @Test(invocationCount = 50, threadPoolSize = 10)
    void addOrRemoveCoordAddr() {
        try {
            if ( isStandAlone() )
                return;
            Sequoiadb sdb = ds2.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
            synchronized ( this ) {
                if ( addrList.isEmpty() ) {
                    super.getAddrList();
                }
            }

            for ( int i = 0; i < this.addrList.size(); ++i ) {
                ds2.addCoord( this.addrList.get( i ).getHostName() + ":"
                        + this.addrList.get( i ).getPort() );
            }
            InetSocketAddress inCoordAddr = getInCoordAddr();
            for ( int i = 0; i < this.addrList.size(); ++i ) {
                InetSocketAddress curAddr = this.addrList.get( i );
                if ( inCoordAddr.equals( curAddr ) )
                    continue;
                ds2.removeCoord(
                        curAddr.getHostName() + ":" + curAddr.getPort() );
            }

            ds2.releaseConnection( sdb );
            sdb = ds2.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
            Thread.sleep( random.nextInt( 50 ) );
            ds2.releaseConnection( sdb );
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() );
        }
    }

    /**
     * 并发禁用，启用连接池
     */
    @Test(invocationCount = 10, threadPoolSize = 5)
    void enableORDisabledDataSource() {
        try {
            ds3.disableDatasource();
            Thread.sleep( random.nextInt( 30 ) );
            ds3.enableDatasource();

            Sequoiadb sdb = ds3.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
            ds3.releaseConnection( sdb );
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            // Assert.assertTrue(false, e.getMessage());
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    /**
     * 申请连接和关闭连接池并发
     */
    @Test(invocationCount = 50, threadPoolSize = 10)
    void getConnectionOrClose() {
        try {
            Sequoiadb sdb = ds4.getConnection();
            // Assert.assertEquals(sdb.isValid(), true);
            Thread.sleep( random.nextInt( 10 ) );
            ds4.releaseConnection( sdb );
            ds4.close();
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }

    /**
     * 更改连接池选项和关闭连接池并发
     */
    @Test(invocationCount = 50, threadPoolSize = 10)
    void updateOptionAndClose() {
        try {
            SequoiadbOption option = new SequoiadbOption();
            option.setMaxIdleCount( 20 );
            ds5.updateDatasourceOptions( option );
            Sequoiadb sdb = ds5.getConnection();
            // Assert.assertEquals(sdb.isValid(), true);
            ds5.releaseConnection( sdb );
            Thread.sleep( random.nextInt( 10 ) );
            ds5.close();
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }

    /**
     * 禁用连接池和更新连接池选项并发
     */
    @Test(invocationCount = 50, threadPoolSize = 10)
    void updateOptionAfterDisabled() {
        try {
            int maxNum = 0;
            Sequoiadb sdb = null;
            maxNum = ds6.getDatasourceOptions().getMaxCount();

            if ( null != option ) {
                maxNum = ds6.getDatasourceOptions().getMaxCount();
                Assert.assertEquals( maxNum, 700 );
            }

            sdb = ds6.getConnection();
            Assert.assertEquals( sdb.isValid(), true );

            ds6.disableDatasource();
            synchronized ( this ) {
                if ( null == option ) {
                    SequoiadbOption toption = new SequoiadbOption();
                    toption.setMaxIdleCount( 20 );
                    toption.setMaxCount( 700 );
                    ds6.updateDatasourceOptions( toption );
                    ds6.enableDatasource();
                    toption = ( SequoiadbOption ) ds6.getDatasourceOptions();
                    Assert.assertEquals( toption.getMaxCount(), 700 );
                    Assert.assertEquals( toption.getMaxIdleCount(), 20 );
                    option = toption;
                } else {
                    ds6.enableDatasource();
                }
            }
            if ( sdb != null ) {
                ds6.releaseConnection( sdb );
            }
            Thread.sleep( random.nextInt( 10 ) );
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    private SequoiadbDatasource datasource_concurrent = null;

    @Test(invocationCount = 50, threadPoolSize = 50)
    void getConnectionOfConcurrent() {
        try {
            synchronized ( this ) {
                if ( datasource_concurrent == null ) {
                    super.init();
                    SequoiadbDatasource ds = new SequoiadbDatasource(
                            this.coordAddr, this.userName, this.password,
                            null );
                    datasource_concurrent = ds;
                }
            }

            Sequoiadb sdb = datasource_concurrent.getConnection();
            Assert.assertEquals( sdb.isValid(), true );

            Thread.sleep( 10 );
            datasource_concurrent.releaseConnection( sdb );
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, e.getMessage() );
        }
    }

    private SequoiadbDatasource sdbpools = null;

    @Test(invocationCount = 500, threadPoolSize = 10)
    void getConnectionFromOldInterface() {
        synchronized ( this ) {
            if ( null == sdbpools ) {
                SequoiadbOption sdbOption = new SequoiadbOption();
                sdbOption.setMaxConnectionNum( 500 );
                sdbOption.setMaxIdeNum( 10 );
                sdbOption.setRecheckCyclePeriod( 5 * 1000 );

                ConfigOptions connectOpt = new ConfigOptions();
                connectOpt.setConnectTimeout( 10000 );
                connectOpt.setMaxAutoConnectRetryTime( 0 );

                ArrayList< String > urls = new ArrayList< String >();
                urls.add( this.coordAddr );
                sdbpools = new SequoiadbDatasource( urls, "", "", connectOpt,
                        sdbOption );
            }
        }

        Sequoiadb sdb = null;
        try {
            sdb = sdbpools.getConnection();
            Thread.sleep( random.nextInt( 100 ) );
            Assert.assertEquals( sdb.isValid(), true );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
        sdbpools.close( sdb );
    }

    /**
     * 关闭连接池后增加coord
     */
    @Test(threadPoolSize = 5, invocationCount = 10)
    public void addCoordAddrAfterClose() {
        try {
            if ( isStandAlone() )
                return;
            // Sequoiadb sdb = ds1.getConnection();
            ds7.close();
            // Assert.assertEquals(sdb.isValid(), false);
            for ( int i = 0; i < addrList.size(); ++i ) {
                ds7.addCoord( addrList.get( i ).getHostName() + ":"
                        + addrList.get( i ).getPort() );
            }
        } catch ( BaseException e ) {
            judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }

    /**
     * 关闭连接池删除coord
     */
    @Test(threadPoolSize = 5, invocationCount = 10)
    public void delCoordAddrAfterClose() {
        if ( isStandAlone() )
            return;
        try {
            for ( int i = 0; i < addrList.size(); ++i ) {
                ds8.addCoord( addrList.get( i ).getHostName() + ":"
                        + addrList.get( i ).getPort() );
            }
        } catch ( BaseException e ) {
            // TODO: handle exception
            // e.printStackTrace();
            super.judegeErrCode( "SDB_SYS", e.getErrorCode() );
            // Assert.assertFalse(true, e.getMessage());
        }

        try {
            if ( null == ds8 )
                return;
            ds8.close();
            for ( int i = 0; i < addrList.size(); ++i ) {
                ds8.removeCoord( addrList.get( i ).getHostName() + ":"
                        + addrList.get( i ).getPort() );
            }
            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }

    @Test(threadPoolSize = 5, invocationCount = 10)
    public void closeDatasouceByConcurrent() {
        try {
            ds9.close();
        } catch ( BaseException e ) {
            // TODO: handle exception
            e.printStackTrace();
            Assert.assertFalse( true, e.getMessage() );
        }
    }
}
