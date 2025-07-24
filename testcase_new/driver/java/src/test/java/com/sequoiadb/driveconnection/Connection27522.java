package com.sequoiadb.driveconnection;

<<<<<<< HEAD
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

=======
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

<<<<<<< HEAD
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @author Xu Mingxing
 * @version 1.0
 * @descreption seqDB-27522:SequoiadbDatasource.builder()方式设置serverAddress(List<String>
 *              addressList)
=======
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * @descreption seqDB-27522:SequoiadbDatasource.builder()方式设置serverAddress(List<String>
 *              addressList)
 * @author Xu Mingxing
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 * @date 2022/9/14
 * @updateUser
 * @updateDate
 * @updateRemark
<<<<<<< HEAD
=======
 * @version 1.0
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
 */

public class Connection27522 extends SdbTestBase {
    private SequoiadbDatasource ds = null;
    private Sequoiadb db = null;
    private Sequoiadb sdb = null;
    private String csName = "cs_27522";

<<<<<<< HEAD
    public static ArrayList< String > getCoordUrls( Sequoiadb sdb ) {
        DBCursor cursor = null;
        cursor = sdb.getList( Sequoiadb.SDB_LIST_GROUPS, null, null, null );
        ArrayList< String > coordUrls = new ArrayList< String >();
        while ( cursor.hasNext() ) {
            BasicBSONObject obj = ( BasicBSONObject ) cursor.getNext();
            String groupName = obj.getString( "GroupName" );
            if ( groupName.equals( "SYSCoord" ) ) {
                BasicBSONList group = ( BasicBSONList ) obj.get( "Group" );
                for ( Object temp : group ) {
                    BSONObject nodeifg = ( BSONObject ) temp;
                    String coordHostName = ( String ) nodeifg.get( "HostName" );
                    BasicBSONList service = ( BasicBSONList ) nodeifg
                            .get( "Service" );
                    BSONObject tempservice = ( BSONObject ) service.get( 0 );
                    String coordPort = ( String ) tempservice.get( "Name" );
                    coordUrls.add( coordHostName + ":" + coordPort );
                }
            }
        }
        cursor.close();
        return coordUrls;
    }

=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @Test
    public void test() throws Exception {
<<<<<<< HEAD
        ConfigOptions netOpt = new ConfigOptions();
        netOpt.setConnectTimeout( 2 * 1000 );
        netOpt.setMaxAutoConnectRetryTime( 2 * 1000 );

=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
        // test a：指定所有地址可用
        List< String > correctUrlList = getCoordUrls( db );
        ds = SequoiadbDatasource.builder().serverAddress( correctUrlList )
                .build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test b：指定部分地址可用
        List< String > partCorrectUrlList = new ArrayList<>();
        partCorrectUrlList.addAll( Arrays.asList(
                SdbTestBase.hostName + ":" + "10", SdbTestBase.coordUrl ) );
        ds = SequoiadbDatasource.builder().serverAddress( partCorrectUrlList )
<<<<<<< HEAD
                .configOptions( netOpt ).build();
=======
                .build();
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test c：多次调用,最后一次List列表中存在可用地址
        List< String > wrongUrlList = new ArrayList<>();
        wrongUrlList.addAll( Arrays.asList( SdbTestBase.hostName + ":" + "10",
                SdbTestBase.hostName + ":" + "30" ) );
        ds = SequoiadbDatasource.builder().serverAddress( wrongUrlList )
                .serverAddress( correctUrlList ).build();
        sdb = ds.getConnection();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test d：指定List为null、[]
        List< String > wrongList = null;
        try {
            ds = SequoiadbDatasource.builder().serverAddress( wrongList )
                    .build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }
        wrongList = new ArrayList<>();
        try {
            ds = SequoiadbDatasource.builder().serverAddress( wrongList )
                    .build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        // test e：指定所有地址不可用
        try {
            ds = SequoiadbDatasource.builder().serverAddress( wrongUrlList )
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

        // test f：多次调用,最后一次List列表中不存在可用地址
        try {
            ds = SequoiadbDatasource.builder().serverAddress( correctUrlList )
<<<<<<< HEAD
                    .serverAddress( wrongUrlList ).configOptions( netOpt )
                    .build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() && e
                    .getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode() ) {
=======
                    .serverAddress( wrongUrlList ).build();
            sdb = ds.getConnection();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode() ) {
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
                throw e;
            }
        }
    }

<<<<<<< HEAD
=======
    public static ArrayList< String > getCoordUrls( Sequoiadb sdb ) {
        DBCursor cursor = null;
        cursor = sdb.getList( Sequoiadb.SDB_LIST_GROUPS, null, null, null );
        ArrayList< String > coordUrls = new ArrayList< String >();
        while ( cursor.hasNext() ) {
            BasicBSONObject obj = ( BasicBSONObject ) cursor.getNext();
            String groupName = obj.getString( "GroupName" );
            if ( groupName.equals( "SYSCoord" ) ) {
                BasicBSONList group = ( BasicBSONList ) obj.get( "Group" );
                for ( Object temp : group ) {
                    BSONObject nodeifg = ( BSONObject ) temp;
                    String coordHostName = ( String ) nodeifg.get( "HostName" );
                    BasicBSONList service = ( BasicBSONList ) nodeifg
                            .get( "Service" );
                    BSONObject tempservice = ( BSONObject ) service.get( 0 );
                    String coordPort = ( String ) tempservice.get( "Name" );
                    coordUrls.add( coordHostName + ":" + coordPort );
                }
            }
        }
        cursor.close();
        return coordUrls;
    }

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    @AfterClass
    public void tearDown() {
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