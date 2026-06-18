package com.sequoiadb.driveconnection.serial;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.UserConfig;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @descreption seqDB-27524:SequoiadbDatasource()方式设置configOption(ConfigOption
 *              option)
 * @author Xu Mingxing
 * @date 2022/9/14
 * @updateUser
 * @updateDate
 * @updateRemark
 * @version 1.0
 */

public class Connection27524 extends SdbTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb db = null;
    private Sequoiadb sdb = null;
    private String csName = "cs_27524";

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void test() throws Exception {
        // test a：不开启SSL,指定setUseSSL()
        // 客户端开启 SSL 而服务端未开启，建连应失败并返回 SDB_NETWORK。
        // 使用较短的连接超时，确保 SSL 握手在连接池等待超时（默认 5s）之前
        // 快速失败，从而拿到 SDB_NETWORK 而非 SDB_TIMEOUT。
        ConfigOptions sslMismatchOpt = new ConfigOptions();
        sslMismatchOpt.setUseSSL( true );
        sslMismatchOpt.setConnectTimeout( 2000 );
        sslMismatchOpt.setMaxAutoConnectRetryTime( 2000 );
        try {
            ds = SequoiadbDatasource.builder()
                    .serverAddress( SdbTestBase.coordUrl )
                    .userConfig( new UserConfig() )
                    .configOptions( sslMismatchOpt ).build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() ) {
                throw e;
            }
        } finally {
            if ( ds != null ) {
                ds.close();
                ds = null;
            }
        }

        // test b：开启SSL,指定setUseSSL()
        ConfigOptions configOption = new ConfigOptions();
        configOption.setUseSSL( true );
        BSONObject sslConf = new BasicBSONObject();
        sslConf.put( "usessl", true );
        db.updateConfig( sslConf );
        ds = SequoiadbDatasource.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( new UserConfig() ).configOptions( configOption )
                .build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test c：随机覆盖其他set接口
        configOption.setUseNagle( true );
        ds = SequoiadbDatasource.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( new UserConfig() ).configOptions( configOption )
                .build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );
    }

    @AfterClass
    public void tearDown() {
        BSONObject sslConf = new BasicBSONObject();
        sslConf.put( "usessl", 1 );
        BSONObject options = new BasicBSONObject();
        options.put( "Global", true );
        db.deleteConfig( sslConf, options );
        if ( ds != null ) {
            ds.close();
        }
        if ( db != null ) {
            db.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}