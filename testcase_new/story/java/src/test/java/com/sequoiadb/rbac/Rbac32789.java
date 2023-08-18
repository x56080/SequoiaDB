package com.sequoiadb.rbac;

import com.sequoiadb.base.*;
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

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32789:角色Resource指定为集合空间，重命名集合空间
 *              seqDB-32790:角色Resource指定为集合，重建集合空间
 * @Author liuli
 * @Date 2023.08.18
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.18
 * @version 1.10
 */
public class Rbac32789 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String rootUser = "sdbadmin_32789";
    private String rootPasswd = "sdbadmin_32789";
    private String user = "user_32789";
    private String password = "passwd_32789";
    private String roleName = "role_32789";
    private String csName = "cs_32789";
    private String csNameNew = "cs_new_32789";
    private String clName = "cl_32789";

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
        testAccessControl( sdb );
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
                "alterCL", "createIndex", "dropIndex", "truncate" };
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

                // 重命名集合空间
                sdb.renameCollectionSpace( csName, csNameNew );

                // 没有权限获取新的集合空间
                try {
                    userSdb.getCollectionSpace( csNameNew );
                    Assert.fail( "should error but success" );
                } catch ( BaseException e ) {
                    Assert.assertEquals( e.getErrorCode(),
                            SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
                }

                // 再次重命名回原始名称
                sdb.renameCollectionSpace( csNameNew, csName );
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

                // 删除集合空间后重建同名集合空间
                sdb.dropCollectionSpace( csName );
                sdb.createCollectionSpace( csName ).createCollection( clName );
                userCL = userSdb.getCollectionSpace( csName )
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
                if ( userSdb != null ) {
                    userSdb.close();
                }
                sdb.removeUser( user, password );
                sdb.dropRole( roleName );
            }
        }
    }
}