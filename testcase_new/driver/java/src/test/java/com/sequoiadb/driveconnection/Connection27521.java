package com.sequoiadb.driveconnection;

<<<<<<< HEAD
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.ConfigOptions;
=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
<<<<<<< HEAD

/**
 * @author xumingxing
 * @version 1.0
 * @Description seqDB-27521:SequoiadbDatasource.builder()方式设置serverAddress(String
 *              address)
 * @Date 2022.09.14
=======
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-27521:SequoiadbDatasource.builder()方式设置serverAddress(String
 *              address)
 * @author xumingxing
 * @Date 2022.09.14
 * @version 1.0
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 */

public class Connection27521 extends SdbTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb sdb = null;
    private String csName = "cs_27521";

    @BeforeClass
    public void setUp() {
    }

    @Test
    public void test() throws Exception {
<<<<<<< HEAD
        ConfigOptions netOpt = new ConfigOptions();
        netOpt.setConnectTimeout( 2 * 1000 );
        netOpt.setMaxAutoConnectRetryTime( 2 * 1000 );

=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
        // test a：指定可用地址
        ds = SequoiadbDatasource.builder().serverAddress( SdbTestBase.coordUrl )
                .build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );
        // 关闭连接后执行操作
        sdb.close();
        try {
            sdb.createCollectionSpace( csName );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NOT_CONNECTED
                    .getErrorCode() ) {
                throw e;
            }
        }

        // test b：多次调用,指定最后一次为可用地址
        String wrongUrl = SdbTestBase.hostName + ":" + "10";
        ds = SequoiadbDatasource.builder().serverAddress( wrongUrl )
                .serverAddress( SdbTestBase.coordUrl ).build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test c：指定地址为null、""
        wrongUrl = null;
        try {
            ds = SequoiadbDatasource.builder().serverAddress( wrongUrl )
                    .build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        wrongUrl = "";
        try {
            ds = SequoiadbDatasource.builder().serverAddress( wrongUrl )
                    .build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        // test d：指定不可用地址
        wrongUrl = SdbTestBase.hostName + ":" + "30";
        try {
            ds = SequoiadbDatasource.builder().serverAddress( wrongUrl )
<<<<<<< HEAD
                    .configOptions( netOpt ).build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() && e
                    .getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode() ) {
=======
                    .build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() ) {
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
                throw e;
            }
        }

        // test e：多次调用,指定最后一次为不可用地址
        try {
            ds = SequoiadbDatasource.builder()
                    .serverAddress( SdbTestBase.coordUrl )
<<<<<<< HEAD
                    .serverAddress( wrongUrl ).configOptions( netOpt ).build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() && e
                    .getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode() ) {
=======
                    .serverAddress( wrongUrl ).build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() ) {
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( ds != null ) {
            ds.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}