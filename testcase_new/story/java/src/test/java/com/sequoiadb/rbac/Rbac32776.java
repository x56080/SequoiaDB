package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32776:创建角色指定Resource为集合，Actions包含多个集合操作
 * @Author liuli
 * @Date 2023.08.15
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.15
 * @version 1.10
 */
public class Rbac32776 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String rootUser = "sdbadmin_32776";
    private String rootPasswd = "sdbadmin_32776";
    private String user = "user_32776";
    private String password = "passwd_32776";
    private String roleName = "role_32776";
    private String csName = "cs_32776";
    private String clName = "cl_32776";

    @BeforeClass
    public void setUp() {
        Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db1 ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        Object options = JSON.parse( "{Roles:['_root']}" );
        db1.createUser( rootUser, rootPasswd, ( BSONObject ) options );
        db1.close();
        sdb = new Sequoiadb( SdbTestBase.coordUrl, rootUser, rootPasswd );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        for ( int i = 0; i < 10; i++ ) {
            testAccessControl( sdb );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            sdb.removeUser( rootUser, rootPasswd );
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate", "testCL" };
        // 随机取2个action
        String[] randomActions = RbacUtils.getRandomActions( actions, 2 );
        BSONObject role = null;
        String action = RbacUtils.arrayToCommaSeparatedString( randomActions );
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + clName + "'}, Actions: [" + action + "] }"
                + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        System.out.println( "roleStr -- " + roleStr );
        role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password );
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection userCL = userSdb.getCollectionSpace( csName )
                .getCollection( clName );

        try {
            for ( String act : actions ) {
                if ( action.contains( act ) ) {
                    System.out.println( "action -- " + act );
                    switch ( act ) {
                    case "find":
                        System.out.println( "action find" );
                        RbacUtils.findActionSupportCommand( sdb, csName, clName,
                                userCL, false );
                        break;
                    case "insert":
                        System.out.println( "action insert" );
                        RbacUtils.insertActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "update":
                        System.out.println( "action update" );
                        RbacUtils.updateActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "remove":
                        System.out.println( "action remove" );
                        RbacUtils.removeActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "getDetail":
                        System.out.println( "action getDetail" );
                        RbacUtils.getDetailActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "alterCL":
                        System.out.println( "action alterCL" );
                        RbacUtils.alterCLActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "createIndex":
                        System.out.println( "action createIndex" );
                        RbacUtils.createIndexActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "dropIndex":
                        System.out.println( "action dropIndex" );
                        RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "truncate":
                        System.out.println( "action truncate" );
                        userCL.truncate();
                        break;
                    default:
                        break;
                    }
                }

                if ( !action.contains( act ) ) {
                    System.out.println( "!action -- " + act );
                    switch ( act ) {
                    case "find":
                        System.out.println( "!action find" );
                        try {
                            userCL.queryOne();
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            Assert.assertEquals( e.getErrorCode(),
                                    SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                        }
                        break;
                    case "insert":
                        System.out.println( "!action insert" );
                        try {
                            userCL.insertRecord(
                                    new BasicBSONObject( "a", 1 ) );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            Assert.assertEquals( e.getErrorCode(),
                                    SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                        }
                        break;
                    case "update":
                        System.out.println( "!action update" );
                        try {
                            userCL.updateRecords( new BasicBSONObject( "a", 1 ),
                                    new BasicBSONObject( "$set",
                                            new BasicBSONObject( "a", 2 ) ) );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            Assert.assertEquals( e.getErrorCode(),
                                    SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                        }
                        break;
                    case "remove":
                        System.out.println( "!action remove" );
                        try {
                            userCL.deleteRecords(
                                    new BasicBSONObject( "a", 2 ) );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            Assert.assertEquals( e.getErrorCode(),
                                    SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                        }
                        break;
                    case "getDetail":
                        // 与find权限支持操作重复
                        break;
                    case "alterCL":
                        System.out.println( "!action alterCL" );
                        try {
                            userCL.alterCollection(
                                    new BasicBSONObject( "ReplSize", -1 ) );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            Assert.assertEquals( e.getErrorCode(),
                                    SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                        }
                        break;
                    case "createIndex":
                        System.out.println( "!action createIndex" );
                        try {
                            String indexName = "index_" + clName;
                            userCL.createIndex( indexName,
                                    new BasicBSONObject( "a", 1 ), null );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            Assert.assertEquals( e.getErrorCode(),
                                    SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                        }
                        break;
                    case "dropIndex":
                        System.out.println( "!action dropIndex" );
                        String indexName = "index_" + clName;
                        rootCL.createIndex( indexName,
                                new BasicBSONObject( "a", 1 ), null );
                        try {
                            userCL.dropIndex( indexName );
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            Assert.assertEquals( e.getErrorCode(),
                                    SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                        } finally {
                            rootCL.dropIndex( indexName );
                        }
                        break;
                    case "truncate":
                        System.out.println( "!action truncate" );
                        try {
                            userCL.truncate();
                            Assert.fail( "should error but success" );
                        } catch ( BaseException e ) {
                            Assert.assertEquals( e.getErrorCode(),
                                    SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        } finally {
            userSdb.close();
            sdb.removeUser( user, password );
            sdb.dropRole( roleName );
        }
    }
}