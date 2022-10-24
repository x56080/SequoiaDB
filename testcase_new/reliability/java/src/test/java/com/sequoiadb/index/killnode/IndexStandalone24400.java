package com.sequoiadb.index.killnode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupWrapper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.index.IndexUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @description seqDB-24400 :: 切分表创建本地索引，部分数据节点异常
 * @author wuyan
 * @date 2021.10.15
 * @version 1.10
 */

public class IndexStandalone24400 extends SdbTestBase {
    private boolean runSuccess = false;
    private DBCollection dbcl;
    private String srcGroupName;
    private String destGroupName;
    private BasicBSONObject srcNodeInfo;
    private BasicBSONObject destNodeInfo;
    private GroupMgr groupMgr;
    private Sequoiadb sdb;
    private String clName = "index_cl_24400";
    private String indexName = "index24400";
    private int recsNum = 50000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass()
    private void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness return false" );
        }

        List< GroupWrapper > grouplist = groupMgr.getAllDataGroup();
        srcGroupName = grouplist.get( 0 ).getGroupName();
        destGroupName = grouplist.get( 1 ).getGroupName();
        System.out.println(
                "split srcRG:" + srcGroupName + " destRG:" + destGroupName );

        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        options.put( "Group", srcGroupName );
        dbcl = cs.createCollection( clName, options );
        insertRecords = IndexUtils.insertData( dbcl, recsNum );
        dbcl.split( srcGroupName, destGroupName, 80 );
    }

    @Test
    private void test() throws Exception {
        srcNodeInfo = getGroupOneNodeName( sdb, srcGroupName );
        destNodeInfo = getGroupOneNodeName( sdb, destGroupName );
        String srcNodeName = srcNodeInfo.getString( "hostName" ) + ":"
                + srcNodeInfo.getString( "svcName" );
        String destNodeName = destNodeInfo.getString( "hostName" ) + ":"
                + destNodeInfo.getString( "svcName" );

        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                destNodeInfo.getString( "hostName" ),
                destNodeInfo.getString( "svcName" ), 3 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask(
                new CreateIndexTask( indexName, srcNodeName, destNodeName ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check cluster
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "failed to restore business" );

        // check results
        checkStandaloneIndexOnNode( sdb, SdbTestBase.csName, clName, indexName,
                srcNodeName, true );
        boolean isExistTask = isExistSuccTask( sdb, SdbTestBase.csName, clName,
                indexName, destNodeName );
        if ( !isExistTask ) {
            checkStandaloneIndexOnNode( sdb, SdbTestBase.csName, clName,
                    indexName, destNodeName, false );
            // 再次创建相同索引
            createIndexStandAlone( dbcl, indexName, destNodeName, null );
            checkStandaloneIndexOnNode( sdb, SdbTestBase.csName, clName,
                    indexName, destNodeName, true );
        } else {
            // 可能出现节点异常恢复后才触发创建索引任务，则检查节点都存在索引
            checkStandaloneIndexOnNode( sdb, SdbTestBase.csName, clName,
                    indexName, destNodeName, true );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            try {
                CollectionSpace cs = sdb
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( clName );
            } finally {
                if ( sdb != null ) {
                    sdb.close();
                }
            }
        }
    }

    private class CreateIndexTask extends OperateTask {
        private String indexName;
        private String nodeName1;
        private String nodeName2;

        private CreateIndexTask( String indexName, String nodeName1,
                String nodeName2 ) {
            this.indexName = indexName;
            this.nodeName1 = nodeName1;
            this.nodeName2 = nodeName2;
        }

        @Override
        public void exec() throws Exception {
            {
                System.out.println( new Date() + " "
                        + this.getClass().getName().toString() );

                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    DBCollection cl = db
                            .getCollectionSpace( SdbTestBase.csName )
                            .getCollection( clName );
                    createIndexStandAlone( dbcl, indexName, nodeName1,
                            nodeName2 );
                } catch ( BaseException e ) {
                    System.out.println( "----e---" + e.getErrorType() );
                    if ( e.getErrorType() != SDBError.SDB_COORD_REMOTE_DISC
                            .getErrorType()
                            && e.getErrorType() != SDBError.SDB_CLS_FULL_SYNC
                                    .getErrorType()
                            && e.getErrorType() != SDBError.SDB_COORD_NOT_ALL_DONE
                                    .getErrorType() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private void createIndexStandAlone( DBCollection cl, String indexName,
            String nodeName1, String nodeName2 ) {
        BSONObject indexKeys = new BasicBSONObject();
        indexKeys.put( "testno", 1 );
        BSONObject indexAttr = new BasicBSONObject();
        indexAttr.put( "Standalone", true );
        BSONObject arr = new BasicBSONList();
        arr.put( "0", nodeName1 );
        // nodeName2为null时，只指定nodename1创建索引
        if ( nodeName2 != null ) {
            arr.put( "1", nodeName2 );
        }
        BSONObject option = new BasicBSONObject();
        option.put( "NodeName", arr );
        cl.createIndex( indexName, indexKeys, indexAttr, option );
    }

    private boolean isExistSuccTask( Sequoiadb db, String csName, String clName,
            String indexName, String nodeName ) {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", "Create index" );
        matcher.put( "NodeName", nodeName );
        matcher.put( "IndexName", indexName );
        matcher.put( "ResultCode", 0 );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TASKS, matcher,
                null, null );

        BSONObject taskInfo = null;
        boolean isExistSuccTask = false;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            isExistSuccTask = true;
        }
        cursor.close();
        return isExistSuccTask;
    }

    private void checkStandaloneIndexOnNode( Sequoiadb db, String csName,
            String clName, String indexName, String indexNodeName,
            boolean isExist ) {
        try ( Sequoiadb dataDB = new Sequoiadb( indexNodeName, "", "" )) {
            DBCollection dbcl = dataDB.getCollectionSpace( csName )
                    .getCollection( clName );
            boolean actIsExistIndex = dbcl.isIndexExist( indexName );
            Assert.assertEquals( actIsExistIndex, isExist,
                    "--check nodeName =" + indexNodeName );
        }
    }

    private BasicBSONObject getGroupOneNodeName( Sequoiadb db,
            String groupName ) {

        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        nodeAddrs = CommLib.getGroupNodes( db, groupName );
        Random random = new Random();
        int serialNum = random.nextInt( nodeAddrs.size() );
        return nodeAddrs.get( serialNum );
    }
}
