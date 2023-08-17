package com.sequoiadb.rbac;

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32788:创建角色指定Resource为集合空间，Actions指定为集合空间操作
 * @Author liuli
 * @Date 2023.08.17
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.17
 * @version 1.10
 */
public class Rbac32788 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String rootUser = "sdbadmin_32788";
    private String rootPasswd = "sdbadmin_32788";
    private String user = "user_32788";
    private String password = "passwd_32788";
    private String roleName = "role_32788";
    private String csName = "cs_32788";
    private String clName = "cl_32788";

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

    @Test(enabled = false)
    public void test() throws Exception {
        testAccessControl( sdb );
        testTestCLAction( sdb );
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
                "alterCL", "createIndex", "dropIndex", "truncate","alterCS",
                "createCL","dropCL","renameCL","listCollections"};
        BSONObject role = null;
        for ( String action : actions ) {
            Sequoiadb userSdb = null;
            try {
                // 需要具备testCS和testCL权限
                String roleStr = "{Role:'" + roleName
                        + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'"
                        + clName + "'}, Actions: ['" + action + "'] }"
                        + ",{ Resource: { cs: '" + csName
                        + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
                System.out.println( "roleStr -- " + roleStr );
                role = ( BSONObject ) JSON.parse( roleStr );
                sdb.createRole( role );
                sdb.createUser( user, password, ( BSONObject ) JSON
                        .parse( "{Roles:['" + roleName + "']}" ) );
                userSdb = new Sequoiadb( SdbTestBase.coordUrl, user, password );
                DBCollection userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clName );
                switch ( action ) {
                case "find":
                    RbacUtils.findActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "insert":
                    RbacUtils.insertActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "update":
                    RbacUtils.updateActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "remove":
                    RbacUtils.removeActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "getDetail":
                    RbacUtils.getDetailActionSupportCommand( sdb, csName,
                            clName, userCL, true );
                    break;
                case "alterCL":
                    RbacUtils.alterCLActionSupportCommand( sdb, csName, clName,
                            userCL, true );
                    break;
                case "createIndex":
                    RbacUtils.createIndexActionSupportCommand( sdb, csName,
                            clName, userCL, true );
                    break;
                case "dropIndex":
                    RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                            clName, userCL, true );
                    break;
                case "truncate":
                    userCL.truncate();
                    break;
                default:
                    break;
                }
            } finally {
                userSdb.close();
                sdb.removeUser( user, password );
                sdb.dropRole( roleName );
            }
        }
    }

    private void testTestCLAction( Sequoiadb sdb ) {
        String action = "testCL";
        BSONObject role = null;
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + clName + "'}, Actions: ['" + action
                + "'] }" + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS'] }] }";
        System.out.println( "roleStr -- " + roleStr );
        role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password );
        try {
            // 支持testCL权限
            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            DBCollection userCL = userCS.getCollection( clName );

            try {
                userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }

            try {
                DBCursor cursor = userSdb.getSnapshot(
                        Sequoiadb.SDB_SNAP_DATABASE, new BasicBSONObject(),
                        null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }
        } finally {
            userSdb.close();
            sdb.removeUser( user, password );
            sdb.dropRole( roleName );
        }
    }
}