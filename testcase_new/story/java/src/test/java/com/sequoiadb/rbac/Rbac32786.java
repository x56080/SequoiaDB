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
 * @Description seqDB-32786:创建角色指定Resource为集合空间，Actions指定为集合操作
 *              seqDB-32793:创建角色Resource指定为集合空间，Actions指定为集群操作
 * @Author liuli
 * @Date 2023.08.17
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.17
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32786 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32786";
    private String password = "passwd_32786";
    private String roleName = "role_32786";
    private String csName = "cs_32786";
    private String clName = "cl_32786_";
    private int clNum = 10;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        // 创建多个集合
        for ( int i = 0; i < clNum; i++ ) {
            cs.createCollection( clName + i );
        }
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
        testTestCLAction( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate" };
        BSONObject role = null;
        for ( String action : actions ) {
            // 需要具备testCS和testCL权限
            String roleStr = "{Role:'" + roleName
                    + "',Privileges:[{Resource:{ cs:'" + csName
                    + "',cl:''}, Actions: ['" + action + "'] }"
                    + ",{ Resource: { cs: '" + csName
                    + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
            System.out.println( "roleStr -- " + roleStr );
            role = ( BSONObject ) JSON.parse( roleStr );
            sdb.createRole( role );
            Sequoiadb userSdb = null;
            try {
                sdb.createUser( user, password, ( BSONObject ) JSON
                        .parse( "{Roles:['" + roleName + "']}" ) );
                userSdb = new Sequoiadb( SdbTestBase.coordUrl, user, password );
                for ( int i = 0; i < clNum; i++ ) {
                    String clName = this.clName + i;
                    DBCollection userCL = userSdb.getCollectionSpace( csName )
                            .getCollection( clName );
                    switch ( action ) {
                    case "find":
                        RbacUtils.findActionSupportCommand( sdb, csName, clName,
                                userCL, true );
                        break;
                    case "insert":
                        RbacUtils.insertActionSupportCommand( sdb, csName,
                                clName, userCL, true );
                        break;
                    case "update":
                        RbacUtils.updateActionSupportCommand( sdb, csName,
                                clName, userCL, true );
                        break;
                    case "remove":
                        RbacUtils.removeActionSupportCommand( sdb, csName,
                                clName, userCL, true );
                        break;
                    case "getDetail":
                        RbacUtils.getDetailActionSupportCommand( sdb, csName,
                                clName, userCL, true );
                        break;
                    case "alterCL":
                        RbacUtils.alterCLActionSupportCommand( sdb, csName,
                                clName, userCL, true );
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

    private void testTestCLAction( Sequoiadb sdb ) {
        String action = "testCL";
        BSONObject role = null;
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: ['" + action + "'] }"
                + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS'] }] }";
        System.out.println( "roleStr -- " + roleStr );
        role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            // 支持testCL权限
            CollectionSpace userCS = userSdb.getCollectionSpace( csName );
            for ( int i = 0; i < clNum; i++ ) {
                String clName = this.clName + i;
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
            }
        } finally {
            sdb.removeUser( user, password );
            sdb.dropRole( roleName );
        }
    }
}