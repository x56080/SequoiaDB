package com.sequoiadb.datasource;

import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Test;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.exception.BaseException;


public class ConcurrentTest7599_7604 extends DataSourceTestBase {
    private SequoiadbDatasource ds = null;
    private DatasourceOptions option = null;
    private Random random = new Random();
    private AtomicInteger count = new AtomicInteger(0);
    private Integer sync = new Integer(0);

    @BeforeMethod
    public void createDatasource() {
        try {
            super.init();
            count.incrementAndGet() ;
            if (ds == null) {
               synchronized(sync) {
                   if (ds == null) {
                       DatasourceOptions sdbOption = new DatasourceOptions();
                       sdbOption.setMaxCount( 500 );
                       sdbOption.setMaxIdleCount( 10 );
                       sdbOption.setCheckInterval( 5 * 1000  );
                       
                       ConfigOptions connectOpt = new ConfigOptions();
                       connectOpt.setConnectTimeout( 10000 );
                       connectOpt.setMaxAutoConnectRetryTime( 0 );
                       
                       ArrayList< String > urls = new ArrayList< String >();
                       urls.add( this.coordAddr );
                       ds = new SequoiadbDatasource( urls, userName, password,
                         connectOpt, sdbOption );
                   }
               }
            }
            
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @AfterMethod
    public void closeDatasource() {
        try {
            if ( ds != null && count.decrementAndGet() == 0) {
                synchronized(sync) {
                    ds.close();
                    ds = null ;
                }
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

    }

    /**
     * 并发申请释放连接
     * @throws InterruptedException 
     * @throws BaseException 
     */
    @Test(invocationCount = 40, threadPoolSize = 4)
    void getConnection() throws InterruptedException  {
        Sequoiadb sdb = ds.getConnection();
        Assert.assertEquals( sdb.isValid(), true );
        Thread.sleep( random.nextInt( 10 ) );
        ds.releaseConnection( sdb );
    }

    /**
     * 增加或者删除coordAddr并发
     */
    @Test(invocationCount = 20, threadPoolSize = 4)
    void addOrRemoveCoordAddr() {
        try {
            if ( isStandAlone() )
                return;
            Sequoiadb sdb = ds.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
            synchronized ( this ) {
                if ( addrList.isEmpty() ) {
                    super.getAddrList();
                }
            }

            for ( int i = 0; i < this.addrList.size(); ++i ) {
                ds.addCoord( this.addrList.get( i ).getHostName() + ":"
                        + this.addrList.get( i ).getPort() );
            }
            InetSocketAddress inCoordAddr = getInCoordAddr();
            for ( int i = 0; i < this.addrList.size(); ++i ) {
                InetSocketAddress curAddr = this.addrList.get( i );
                if ( inCoordAddr.equals( curAddr ) )
                    continue;
                ds.removeCoord(
                        curAddr.getHostName() + ":" + curAddr.getPort() );
            }

            ds.releaseConnection( sdb );
            sdb = ds.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
            Thread.sleep( random.nextInt( 50 ) );
            ds.releaseConnection( sdb );
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    /**
     * 并发禁用，启用连接池
     */
    @Test(invocationCount = 8, threadPoolSize = 4)
    void enableORDisabledDataSource() {
        try {
            ds.disableDatasource();
            Thread.sleep( random.nextInt( 30 ) );
            ds.enableDatasource();

            Sequoiadb sdb = ds.getConnection();
            Assert.assertEquals( sdb.isValid(), true );
            ds.releaseConnection( sdb );
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
    @Test(invocationCount = 12, threadPoolSize = 4)
    void getConnectionOrClose() {
        try {
            ds.getConnection();
            // Assert.assertEquals(sdb.isValid(), true);
            Thread.sleep( random.nextInt( 10 ) );
            ds.close();
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }

    /**
     * 更改连接池选项和关闭连接池并发
     */
    @Test(invocationCount = 12, threadPoolSize = 4)
    void updateOptionAndClose() {
        try {
            DatasourceOptions option = new DatasourceOptions();
            option.setMaxIdleCount( 20 );
            ds.updateDatasourceOptions( option );
            Sequoiadb sdb = ds.getConnection();
            // Assert.assertEquals(sdb.isValid(), true);
            ds.releaseConnection( sdb );
            Thread.sleep( random.nextInt( 10 ) );
            ds.close();
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }

    /**
     * 禁用连接池和更新连接池选项并发
     */
    @Test(invocationCount = 40, threadPoolSize = 4)
    void updateOptionAfterDisabled() {
        try {
            int maxNum = 0;
            Sequoiadb sdb = null;
            maxNum = ds.getDatasourceOptions().getMaxCount();

            if ( null != option ) {
                maxNum = ds.getDatasourceOptions().getMaxCount();
                Assert.assertEquals( maxNum, 700 );
            }

            sdb = ds.getConnection();
            Assert.assertEquals( sdb.isValid(), true );

            ds.disableDatasource();
            synchronized ( this ) {
                if ( null == option ) {
                    DatasourceOptions toption = new DatasourceOptions();
                    toption.setMaxIdleCount( 20 );
                    toption.setMaxCount( 700 );
                    ds.updateDatasourceOptions( toption );
                    ds.enableDatasource();
                    toption = ds.getDatasourceOptions();
                    Assert.assertEquals( toption.getMaxCount(), 700 );
                    Assert.assertEquals( toption.getMaxIdleCount(), 20 );
                    option = toption;
                } else {
                    ds.enableDatasource();
                }
            }
            if ( sdb != null ) {
                ds.releaseConnection( sdb );
            }
            Thread.sleep( random.nextInt( 10 ) );
        } catch ( InterruptedException e ) {
            Assert.assertTrue( false, e.getMessage() );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test(invocationCount = 50, threadPoolSize = 4)
    void getConnectionOfConcurrent() throws InterruptedException {
        Sequoiadb sdb = ds.getConnection();
        Assert.assertEquals( sdb.isValid(), true );
        Thread.sleep( 10 );
    }

    @Test(invocationCount = 40, threadPoolSize = 4)
    void getConnectionFromOldInterface() throws  InterruptedException {
        Sequoiadb sdb = null;
        sdb = ds.getConnection();
        Thread.sleep( random.nextInt( 100 ) );
        Assert.assertEquals( sdb.isValid(), true );
        
        ds.releaseConnection( sdb );
    }

    /**
     * 关闭连接池后增加coord
     */
    @Test(threadPoolSize = 4, invocationCount = 8)
    public void addCoordAddrAfterClose() {
        try {
            if ( isStandAlone() )
                return;
            int num = random.nextInt(addrList.size()) + 1;
            for ( int i = 0; i < num; ++i ) {
                ds.addCoord( addrList.get( i ).getHostName() + ":"
                        + addrList.get( i ).getPort() );
            }
            // Sequoiadb sdb = ds1.getConnection();
            ds.close();
            // Assert.assertEquals(sdb.isValid(), false);
            
        } catch ( BaseException e ) {
            judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }

    /**
     * 关闭连接池删除coord
     */
    @Test(threadPoolSize = 4, invocationCount = 8)
    public void delCoordAddrAfterClose() {
        if ( isStandAlone() )
            return;
        try {
            for ( int i = 0; i < addrList.size(); ++i ) {
                if ( ds != null ) {
                    ds.addCoord( addrList.get( i ).getHostName() + ":"
                        + addrList.get( i ).getPort() );
                }
            }
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_SYS", e.getErrorCode() );
            // Assert.assertFalse(true, e.getMessage());
        }

        try {
            int num = random.nextInt(addrList.size()) + 1;
            for ( int i = 0; i < num; ++i ) {
                if ( ds != null) {
                    ds.removeCoord( addrList.get( i ).getHostName() + ":"
                        + addrList.get( i ).getPort() );
                }
            }
            
            ds.close();
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_SYS", e.getErrorCode() );
        }
    }

    @Test(threadPoolSize = 2, invocationCount = 4)
    public void closeDatasouceByConcurrent() {
        ds.close() ;
    }
}
