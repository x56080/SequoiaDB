package com.sequoiadb.rbac;

import java.util.Random;

import com.sequoiadb.base.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32775:创建角色指定Resource为集合，Actions包含一个集合操作
 * @Author liuli
 * @Date 2023.08.11
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.11
 * @version 1.10
 */
public class Rbac32775 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String rootUser = "sdbadmin_32775";
    private String rootPasswd = "sdbadmin_32775";
    private String user = "user_32775";
    private String password = "passwd_32775";
    private String roleName = "role_32775";
    private String csName = "cs_32775";
    private String clName = "cl_32775";
    private int writeLobSize = 1024;
    private ObjectId oid = null;
    private String indexName1 = "index_32775_1";
    private String indexName2 = "index_32775_2";

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
        DBCollection cl = cs.createCollection( clName );
        cl.insertRecord( new BasicBSONObject( "a", 1 ) );

        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = cl.createLob();
        lob.write( wlobBuff );
        oid = lob.getID();
        lob.close();
        cl.createIndex( indexName1, new BasicBSONObject( "a", 1 ), null );
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
         DBCollection sdbcl =
         sdb.getCollectionSpace(csName).getCollection(clName);
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate", "testCL" };
        BSONObject role = null;
        for ( String action : actions ) {
            String roleStr = "{\"Role\":\"" + roleName
                    + "\",\"Privileges\":[{\"Resource\":{ \"cs\":\"" + csName
                    + "\",\"cl\":\"" + clName + "\"}, \"Actions\": [\"" + action
                    + "\"] }" + ",{ \"Resource\": { \"cs\": \"" + csName
                    + "\", \"cl\": '' }, \"Actions\": ['testCS','testCL'] }] }";
            System.out.println( "roleStr -- " + roleStr );
            role = ( BSONObject ) JSON.parse( roleStr );
            sdb.createRole( role );
            sdb.createUser( user, password, ( BSONObject ) JSON
                    .parse( "{Roles:['" + roleName + "']}" ) );
            Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                    password );
            DBCollection cl = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor cursor = null;
            try {
                switch ( action ) {
                case "find":
                    cursor = cl.query();
                    cursor.getNext();
                    cursor.close();
                    cl.queryOne();
                    cl.getCount();
                    cursor = cl.listLobs();
                    cursor.getNext();
                    cursor.close();
                    cl.getIndexInfo( "$id" );
                    cursor = cl.getIndexes();
                    cursor.getNext();
                    cursor.close();
                    // cursor = cl.snapshotIndexes( null, null, null, null, -1,
                    // -1 );
                    // cursor.getNext();
                    // cursor.close();
                    sdb.analyze( new BasicBSONObject( "Collection",
                            csName + "." + clName ) );
                    cl.getIndexStat( "$id" );
                    break;
                case "insert":
                    cl.insertRecord( new BasicBSONObject( "a", 1 ) );
                    DBLob lob = cl.createLob();
                    lob.write( wlobBuff );
                    lob.close();
                    break;
                case "update":
                    cl.upsertRecords( new BasicBSONObject( "a", 1 ),
                            new BasicBSONObject( "a", 2 ) );
                    cl.upsertRecords( new BasicBSONObject( "a", 1 ),
                            new BasicBSONObject( "a", 2 ) );
                    break;
                case "remove":
                    cl.deleteRecords( new BasicBSONObject( "a", 1 ) );
                    cl.removeLob( oid );
                    break;
                case "getDetail":
                    cl.getCount();
                    cl.getIndexInfo( "$id" );
                    cl.getIndexes();
                    cl.getIndexStat( "$id" );
                    break;
                case "alterCL":
                    // cl.alter( new BasicBSONObject() );
                    break;
                case "createIndex":
                    cl.createIndex( indexName2, new BasicBSONObject( "b", 1 ),
                            null );
                    sdbcl.dropIndex( indexName2 );
                    break;
                case "dropIndex":
                    sdbcl.createIndex( indexName1, new BasicBSONObject( "b", 1 ),
                            null );
                    cl.dropIndex( indexName1 );
                    break;
                case "truncate":
                    cl.truncate();
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

    public static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }
}