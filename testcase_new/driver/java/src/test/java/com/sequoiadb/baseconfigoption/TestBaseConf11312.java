package com.sequoiadb.baseconfigoption;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Descreption seqDB-11312:set/getSoketTimeout()接口测试
 * @Author Cheng Jingjing
 * @CreateDate 2023/4/6
 * @UpdateUser
 * @UpdateDate 2023/4/6
 * @UpdateRemark
 * @Version 1.0
 */
public class TestBaseConf11312 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String clName = "cl_11312";

    @BeforeClass
    public void setUp(){
    }

    @AfterClass
    public void tearDown() {
    }

    @Test
    public void test() {
        ConfigOptions options = new ConfigOptions();
        // 设置socket timeout为1ms
        options.setSocketTimeout( 30 );
        System.out.println( SdbTestBase.coordUrl );
        sdb = Sequoiadb.builder()
                .serverAddress( SdbTestBase.coordUrl )
                .configOptions( options )
                .build();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        try {
            DBCollection cl = cs.createCollection( clName );
            BSONObject obj = new BasicBSONObject();
            obj.put( "key", "value" );
            for ( int i = 0; i < 100; i++ ) {
                cl.insertRecord( obj );
            }
            cl.query();
            cl.truncate();
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode() ) {
                throw e ;
            }
        }
        sdb.close();
    }
}
