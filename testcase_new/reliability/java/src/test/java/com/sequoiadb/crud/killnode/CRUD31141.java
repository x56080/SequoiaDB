package com.sequoiadb.crud.killnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.result.InsertResult;
import com.sequoiadb.commlib.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * @Descreption seqDB-31141:insert过程中，数据节点异常
 * @Author biqin
 * @CreateDate 2023/4/16
 * @UpdateUser biqin
 * @UpdateDate 2023/4/19
 * @UpdateRemark
 * @Version 1.0
 */
public class CRUD31141 extends SdbTestBase {
    private CollectionSpace cs;
    private String groupName;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String csName = "cs_31141";
    private String clName = "cl_31141";
    private int clNum = 2;
    private long insertNum = 100000;
    private String indexName = "index31141";
    private List< BSONObject > expRecord;
    private List< List< BSONObject > > listExpRecord = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cs = sdb.createCollectionSpace( csName );
        groupName = groupMgr.getAllDataGroup().get( 0 ).getGroupName();
        CreateCLAndIndex();
    }

    @Test
    private void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper priNode = dataGroup.getMaster();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( priNode.hostName(),
                priNode.svcName(), 5 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new InsertCL() );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );
        saveCLResult();
        List< NodeWrapper > listNodeWrapper = dataGroup.getNodes();
        for ( NodeWrapper nodeWrapper : listNodeWrapper ) {
            Sequoiadb nodeWrapperDB = nodeWrapper.connect();
            checkInsertResult( nodeWrapperDB );
        }
    }

    @AfterClass
    private void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private InsertResult insertData( DBCollection cl, long insertNum ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        for ( long i = 0; i < insertNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "no", i );
            obj.put( "a", i );
            obj.put( "b", i );
            insertRecord.add( obj );
        }
        return cl.bulkInsert( insertRecord );
    }

    private void saveCLResult() {
        for ( int i = 0; i < clNum; i++ ) {
            String Name = clName + "_" + 0;
            // 协调节点
            DBCollection cl = cs.getCollection( Name );
            DBCursor cursor = cl.query( "", "", "{'a':1}",
                    "{'':'" + indexName + "'}" );
            expRecord = new ArrayList<>();
            while ( cursor.hasNext() ) {
                BSONObject result = cursor.getNext();
                expRecord.add( result );
            }
            cursor.close();
            listExpRecord.add( expRecord );
        }
    }

    private void checkInsertResult( Sequoiadb sdb ) {

        for ( int i = 0; i < clNum; i++ ) {
            String Name = clName + "_" + 0;
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            DBCollection cl = cs.getCollection( Name );
            DBCursor cursor = cl.query( "", "", "{'a':1}",
                    "{'':'" + indexName + "'}" );
            expRecord = listExpRecord.get( i );
            // 数据节点
            int count = 0;
            while ( cursor.hasNext() ) {
                BSONObject result = cursor.getNext();
                Assert.assertEquals( result, expRecord.get( count ) );
                count++;
            }
            Assert.assertEquals( count, expRecord.size() );
            cursor.close();
        }
    }

    private void CreateCLAndIndex() {
        for ( int i = 0; i < clNum; i++ ) {
            String Name = clName + "_" + i;
            DBCollection dbcl = cs.createCollection( Name,
                    new BasicBSONObject( "Group", groupName ) );
            dbcl.createIndex( indexName, "{a:1}", true, false );
        }
    }

    private class InsertCL extends OperateTask {

        @Override
        public void exec() throws Exception {

            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                for ( int i = 0; i < clNum; i++ ) {
                    String Name = clName + "_" + i;
                    DBCollection cl = cs.getCollection( Name );
                    InsertResult insertResult = insertData( cl, insertNum );
                    Assert.assertEquals( insertResult.getInsertNum(),
                            insertNum );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_RTN_NO_PRIMARY_FOUND
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
