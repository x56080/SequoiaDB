package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.exception.BaseException;

public class IndexUtils {
    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int length ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum; i++ ) {
            String keyValue = getRandomString( length );
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", keyValue );
            obj.put( "no", i );
            obj.put( "a", i );
            insertRecord.add( obj );
        }
        dbcl.insert( insertRecord );
        return insertRecord;
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int beginNo, int length ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        for ( int i = beginNo; i < beginNo + recordNum; i++ ) {
            String keyValue = getRandomString( length );
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", keyValue );
            obj.put( "no", i );
            obj.put( "a", i );
            insertRecord.add( obj );
        }
        dbcl.insert( insertRecord );
        return insertRecord;
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum ) {

        return insertData( dbcl, recordNum, 2 );
    }

    public static void checkRecords( DBCollection dbcl,
            List< BSONObject > expRecord, String matcher, String hint ) {
        DBCursor cursor = dbcl.query( matcher, "", "{'no':1}", hint );
        int count = 0;

        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            Assert.assertEquals( record, expRecord.get( count++ ) );
        }
        cursor.close();
        Assert.assertEquals( count, expRecord.size() );
    }

    public static String getRandomString( int length ) {
        String str = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^asssgggg!@#$";
        StringBuffer sbBuffer = new StringBuffer();
        // random generation 80-length string.
        Random random = new Random();
        StringBuffer subBuffer = new StringBuffer();
        int strLen = str.length();
        for ( int i = 0; i < strLen; i++ ) {
            int number = random.nextInt( strLen );
            subBuffer.append( str.charAt( number ) );
        }

        // generate a string at a specified length by subBuffer
        int times = length / str.length();
        for ( int i = 0; i < times; i++ ) {
            sbBuffer.append( subBuffer );
        }
        int subTimes = length % str.length();
        if ( subTimes != 0 ) {
            sbBuffer.append( str.substring( 0, subTimes ) );
        }
        return sbBuffer.toString();
    }

    // 获取cl所在的所有数据节点
    public static ArrayList< String[] > getClNodes( Sequoiadb sdb,
            String csname, String clname ) {
        DBCursor snapshot = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                "{'Name': '" + csname + "." + clname + "'}", "", "" );
        ArrayList< String > nodegroups = new ArrayList< String >();
        BSONObject next = snapshot.getNext();
        ArrayList< BSONObject > groups = ( ArrayList< BSONObject > ) next
                .get( "CataInfo" );
        for ( int i = 0; i < groups.size(); i++ ) {
            nodegroups.add( ( String ) groups.get( i ).get( "GroupName" ) );
        }
        ArrayList< String[] > nodes = new ArrayList< String[] >();
        for ( int i = 0; i < nodegroups.size(); i++ ) {
            DBCursor nodeSnapshot = sdb.getList( Sequoiadb.SDB_LIST_GROUPS,
                    new BasicBSONObject( "GroupName", nodegroups.get( i ) ),
                    null, null );
            ArrayList< BSONObject > group = ( ArrayList< BSONObject > ) nodeSnapshot
                    .getNext().get( "Group" );
            for ( int j = 0; j < group.size(); j++ ) {
                ArrayList< BSONObject > service = ( ArrayList< BSONObject > ) group
                        .get( j ).get( "Service" );
                String[] node = { ( String ) group.get( j ).get( "HostName" ),
                        ( String ) service.get( 0 ).get( "Name" ) };
                nodes.add( node );
            }
        }
        return nodes;
    }

    // 检查索引主备节点一致性
    public static void checkIndexConsistent( Sequoiadb sdb, String csName,
            String clName, String idxName, boolean isexist ) throws Exception {
        // 获取cl所在所有组
        DBCursor snapshot = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                "{'Name': '" + csName + "." + clName + "'}", "", "" );
        BSONObject next = snapshot.getNext();
        ArrayList< BSONObject > groups = ( ArrayList< BSONObject > ) next
                .get( "CataInfo" );
        snapshot.close();
        // 校验lsn是否一致
        for ( int i = 0; i < groups.size(); i++ ) {
            System.out.println( "----islsn:groupname="
                    + ( String ) groups.get( i ).get( "GroupName" ) );
            Assert.assertTrue( isLSNConsistency( sdb,
                    ( String ) groups.get( i ).get( "GroupName" ) ) );
        }
        // 校验主备节点索引信息一致
        if ( isexist ) {
            ArrayList< String[] > clNodes = getClNodes( sdb, csName, clName );
            BSONObject expIndexDef = null;
            for ( int i = 0; i < clNodes.size(); i++ ) {
                try ( Sequoiadb nodeSdb = new Sequoiadb(
                        clNodes.get( i )[ 0 ] + ":" + clNodes.get( i )[ 1 ], "",
                        "" ) ;) {
                    DBCollection dbcl = nodeSdb.getCollectionSpace( csName )
                            .getCollection( clName );
                    DBCursor index = dbcl.getIndex( idxName );
                    BSONObject indexDef = ( BSONObject ) index.getCurrent()
                            .get( "IndexDef" );
                    if ( expIndexDef == null ) {
                        expIndexDef = indexDef;
                    } else {
                        indexDef.removeField( "CreateTime" );
                        indexDef.removeField( "RebuildTime" );
                        indexDef.removeField( "_id" );
                        expIndexDef.removeField( "CreateTime" );
                        expIndexDef.removeField( "RebuildTime" );
                        expIndexDef.removeField( "_id" );
                        Assert.assertEquals( indexDef, expIndexDef );
                    }
                }

            }
        } else {
            ArrayList< String[] > clNodes = getClNodes( sdb, csName, clName );
            for ( int i = 0; i < clNodes.size(); i++ ) {
                try ( Sequoiadb sequoiadb = new Sequoiadb(
                        clNodes.get( i )[ 0 ] + ":" + clNodes.get( i )[ 1 ], "",
                        "" ) ;) {
                    DBCollection dbcl = sequoiadb.getCollectionSpace( csName )
                            .getCollection( clName );
                    dbcl.getIndexInfo( idxName );
                } catch ( BaseException e ) {
                    if ( !( e.getErrorType().equals( "SDB_IXM_NOTEXIST" ) ) ) {
                        e.printStackTrace();
                    }
                } catch ( Exception e ) {
                    e.printStackTrace();
                }
            }
        }
    }

    /**
     * 检查CL主备节点集合CompleteLSN一致
     *
     * @param
     * @return boolean 如果主节点CompleteLSN小于等于备节点CompleteLSN返回true,否则返回false
     * @throws Exception
     * @author luweikang
     */
    public static boolean isLSNConsistency( Sequoiadb db, String groupName )
            throws Exception {
        boolean isConsistency = false;
        List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
        ReplicaGroup rg = db.getReplicaGroup( groupName );

        try ( Sequoiadb masterNode = rg.getMaster().connect()) {
            long completeLSN = -2;
            DBCursor cursor = masterNode.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                    null, "{CompleteLSN: ''}", null );
            if ( cursor.hasNext() ) {
                BasicBSONObject snapshot = ( BasicBSONObject ) cursor.getNext();
                if ( snapshot.containsField( "CompleteLSN" ) ) {
                    completeLSN = ( long ) snapshot.get( "CompleteLSN" );
                }
            } else {
                throw new Exception( masterNode.getNodeName()
                        + " can't not find system snapshot" );
            }
            cursor.close();

            for ( String nodeName : nodeNames ) {
                if ( masterNode.getNodeName().equals( nodeName ) ) {
                    continue;
                }
                isConsistency = false;
                try ( Sequoiadb nodeConn = rg.getNode( nodeName ).connect()) {
                    DBCursor cur = null;
                    long checkCompleteLSN = -3;
                    for ( int i = 0; i < 600; i++ ) {
                        cur = nodeConn.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                                null, "{CompleteLSN: ''}", null );
                        if ( cur.hasNext() ) {
                            BasicBSONObject checkSnapshot = ( BasicBSONObject ) cur
                                    .getNext();
                            if ( checkSnapshot
                                    .containsField( "CompleteLSN" ) ) {
                                checkCompleteLSN = ( long ) checkSnapshot
                                        .get( "CompleteLSN" );
                            }
                        }
                        cur.close();

                        if ( completeLSN <= checkCompleteLSN ) {
                            isConsistency = true;
                            break;
                        }
                        try {
                            Thread.sleep( 1000 );
                        } catch ( InterruptedException e ) {
                            e.printStackTrace();
                        }
                    }
                    if ( !isConsistency ) {
                        System.out.println( "Group [" + groupName
                                + "] node system snapshot is not the same, masterNode "
                                + masterNode.getNodeName() + " CompleteLSN: "
                                + completeLSN + ", " + nodeName
                                + " CompleteLSN: " + checkCompleteLSN );
                    }
                }
            }
        }

        return isConsistency;
    }

    // 获取cl所在所有组名
    public static List< String > getCLGroupNames( Sequoiadb sdb, String csName,
            String clName ) {
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                "{'Name': '" + csName + "." + clName + "'}", "", "" );
        List< String > groupNames = new ArrayList<>();
        while ( cursor.hasNext() ) {
            BSONObject clInfo = cursor.getNext();
            BasicBSONList cataInfo = ( BasicBSONList ) clInfo.get( "CataInfo" );
            for ( int i = 0; i < cataInfo.size(); ++i ) {
                BasicBSONObject groupInfo = ( BasicBSONObject ) cataInfo
                        .get( i );
                String groupName = groupInfo.getString( "GroupName" );
                groupNames.add( groupName );
            }
        }
        cursor.close();
        return groupNames;
    }

    /**
     * @description: 等待任务执行完成
     * @param csName
     * @param clName
     * @param taskTypeDesc
     *            //任务类型
     */
    public static void waitTaskFinish( Sequoiadb sdb, String csName,
            String clName, String taskTypeDesc ) {

        waitTaskFinish( sdb, csName, clName, taskTypeDesc, 1 );
    }

    public static void waitTaskFinish( Sequoiadb sdb, String csName,
            String clName, String taskTypeDesc, int taskNum ) {
        // status取值 0:Ready 9:Finish
        int status = 9;
        int times = 0;
        int sleepTime = 100;
        int maxWaitTimes = 20000;

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );

        while ( true ) {
            DBCursor cursor = sdb.listTasks( matcher, null, null, null );
            int finshTaskNum = 0;
            BSONObject taskInfo = null;
            while ( cursor.hasNext() ) {
                taskInfo = cursor.getNext();
                int actStatus = ( int ) taskInfo.get( "Status" );
                if ( actStatus != status ) {
                    break;
                }
                finshTaskNum++;
            }
            cursor.close();

            if ( finshTaskNum == taskNum ) {
                break;
            } else if ( times * sleepTime > maxWaitTimes ) {
                throw new Error( "waiting task time out! waitTimes="
                        + times * sleepTime + "\ntask=" + taskInfo.toString() );
            }

            try {
                Thread.sleep( sleepTime );
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            times++;
        }
    }

    /**
     * @description: 检查copy任务信息（选择copy子表名、索引名、结果和状态码比较）
     * @param csName
     *            主表所在cs
     * @param mainclName
     *            主表名
     * @param indexNames
     *            检验测索引名,复制索引操作需要传入索引名数组
     * @param subclNames
     *            检验子表名
     * @param resultCode
     *            任务错误码 成功:0 失败:错误码
     * @param status
     *            //任务状态码 完成:9
     */
    @SuppressWarnings("unchecked")
    public static void checkCopyTask( Sequoiadb db, String csName,
            String mainclName, List< String > indexNames,
            List< String > subclNames, int resultCode, int status ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + mainclName );
        matcher.put( "TaskTypeDesc", "Copy index" );
        DBCursor cursor = db.listTasks( matcher, null, null, null );
        BSONObject taskInfo = new BasicBSONObject();
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        // 校验索引名
        List< String > actIndexNames = ( List< String > ) taskInfo
                .get( "IndexNames" );
        System.out.println( "-----actndexenames=" + actIndexNames );
        Collections.sort( actIndexNames );
        Collections.sort( indexNames );
        Assert.assertEquals( actIndexNames, indexNames );

        // 校验结果状态码和结果码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        Assert.assertEquals( actResultCode, resultCode );

        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status );

        // 校验copy子表信息
        List< String > actSubCLNames = ( List< String > ) taskInfo
                .get( "CopyTo" );
        Collections.sort( actSubCLNames );
        Collections.sort( subclNames );
        Assert.assertEquals( actSubCLNames, subclNames );

        // 校验任务数,匹配1条成功任务
        Assert.assertEquals( taskNum, 1 );
    }

    public static void checkCopyTask( Sequoiadb db, String csName,
            String mainclName, List< String > indexNames,
            List< String > subclNames ) {
        checkCopyTask( db, csName, mainclName, indexNames, subclNames, 0, 9 );
    }

    /**
     * @description: 检查create /copy/ drop index任务信息（索引名、组名、结果状态码比较）
     * @param taskTypeDesc
     *            任务类型
     * @param resultCode
     *            任务错误码 成功:0 失败:错误码
     * @param csName
     * @param clName
     * @param indexName
     *            检验测索引名
     */
    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName, int resultCode ) {
        checkIndexTask( db, taskTypeDesc, csName, clName, indexName, resultCode,
                true );
    }

    /**
     * @description: 检查create / drop
     *               index任务信息（通过cl、索引名和任务类型获取任务，检查索引名、组名、结果状态码比较）
     * @param taskTypeDesc
     *            任务类型
     * @param resultCode
     *            任务错误码 成功:0 失败:错误码
     * @param csName
     *            cs名
     * @param clName
     *            cl名
     * @param indexName
     *            检验测索引名
     * @param resultCode
     *            预期任务结果码
     * @param isCheckGroupInfo
     *            判断是否需要校验任务中组信息，设置为true校验任务中展示组，设置为false不需要校验任务中的组
     */

    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName, int resultCode,
            boolean isCheckGroupInfo ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        Assert.assertEquals( taskNum, 1, "index task num should be 1!" );
        // 校验结果状态码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        Assert.assertEquals( actResultCode, resultCode,
                "taskinfo:" + taskInfo );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status, "taskinfo:" + taskInfo );

        boolean isMainTask = taskInfo.containsField( "IsMainTask" );
        if ( !isMainTask && isCheckGroupInfo ) {
            // 校验组信息
            @SuppressWarnings("unchecked")
            List< BSONObject > actGroupInfos = ( List< BSONObject > ) taskInfo
                    .get( "Groups" );
            List< String > groupNames = new ArrayList<>();
            for ( int i = 0; i < actGroupInfos.size(); i++ ) {
                BSONObject groupInfo = actGroupInfos.get( i );
                String groupName = ( String ) groupInfo.get( "GroupName" );
                groupNames.add( groupName );
                int groupResultCode = ( int ) groupInfo.get( "ResultCode" );
                Assert.assertEquals( groupResultCode, resultCode,
                        "check group task error! groupName=" + groupName );
            }

            List< String > expGroupNames = getCLGroupNames( db, csName,
                    clName );
            System.out.println( "----expGroupNames=" + expGroupNames );
            System.out.println( "----groupNames=" + groupNames );
            Collections.sort( expGroupNames );
            Collections.sort( groupNames );
            Assert.assertEquals( groupNames, expGroupNames,
                    "check group error!task=" + taskInfo.toString() );
        }
    }

    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String[] indexNames,
            int[] resultCodes ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "IndexNames", indexNames );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        Assert.assertEquals( taskNum, 1, "index task num should be 1!" );
        // 校验结果状态码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        boolean isEqual = intArrayContainsField( resultCodes, actResultCode );
        Assert.assertTrue( isEqual, "The act resultCode=" + actResultCode
                + ",act resultCodes = " + resultCodes.toString() );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status );

        boolean isMainTask = taskInfo.containsField( "IsMainTask" );
        if ( !isMainTask ) {
            // 校验组信息
            @SuppressWarnings("unchecked")
            List< BSONObject > actGroupInfos = ( List< BSONObject > ) taskInfo
                    .get( "Groups" );
            List< String > groupNames = new ArrayList<>();
            for ( int i = 0; i < actGroupInfos.size(); i++ ) {
                BSONObject groupInfo = actGroupInfos.get( i );
                String groupName = ( String ) groupInfo.get( "GroupName" );
                groupNames.add( groupName );
                int groupResultCode = ( int ) groupInfo.get( "ResultCode" );
                boolean isEqualFlag = intArrayContainsField( resultCodes,
                        groupResultCode );
                Assert.assertTrue( isEqualFlag,
                        "check resultcode error! groupName=" + groupName
                                + ",The act resultCode=" + groupResultCode );

            }

            List< String > expGroupNames = getCLGroupNames( db, csName,
                    clName );
            System.out.println( "----expGroupNames=" + expGroupNames );
            System.out.println( "----groupNames=" + groupNames );
            Collections.sort( expGroupNames );
            Collections.sort( groupNames );
            Assert.assertEquals( groupNames, expGroupNames,
                    "check group error!task=" + taskInfo.toString() );
        }
    }

    /**
     * @description: 检查create / drop index任务信息（索引名、组名、结果状态码比较）
     * @param taskTypeDesc
     *            任务类型
     * @param resultCodes
     *            任务错误码 成功:0 失败:错误码
     * @param csName
     * @param clName
     * @param indexName
     *            检验测索引名
     */
    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName,
            int[] resultCodes ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        Assert.assertEquals( taskNum, 1, "index task num should be 1!" );
        // 校验结果状态码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );

        boolean isEqual = intArrayContainsField( resultCodes, actResultCode );
        Assert.assertTrue( isEqual, "The act resultCode=" + actResultCode
                + ",act resultCodes = " + resultCodes.toString() );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status );

        boolean isMainTask = taskInfo.containsField( "IsMainTask" );
        if ( !isMainTask ) {
            // 校验组信息
            @SuppressWarnings("unchecked")
            List< BSONObject > actGroupInfos = ( List< BSONObject > ) taskInfo
                    .get( "Groups" );
            List< String > groupNames = new ArrayList<>();
            for ( int i = 0; i < actGroupInfos.size(); i++ ) {
                BSONObject groupInfo = actGroupInfos.get( i );
                String groupName = ( String ) groupInfo.get( "GroupName" );
                groupNames.add( groupName );
                int actResultCode1 = ( int ) groupInfo.get( "ResultCode" );
                Assert.assertTrue(
                        intArrayContainsField( resultCodes, actResultCode1 ),
                        "check resultcode error! groupName=" + groupName
                                + ".act resultCode=" + actResultCode1 );
            }

            List< String > expGroupNames = getCLGroupNames( db, csName,
                    clName );
            System.out.println( "----expGroupNames=" + expGroupNames );
            System.out.println( "----groupNames=" + groupNames );
            Collections.sort( expGroupNames );
            Collections.sort( groupNames );
            Assert.assertEquals( groupNames, expGroupNames,
                    "check group error!task=" + taskInfo.toString() );
        }

    }

    /**
     * @description: 判断int数组中是否存在某个值
     * @param intArr
     * @param intValue
     *            需要找的值
     * @return 存在返回true，不存在返回false *
     */
    public static boolean intArrayContainsField( int[] intArr, int intValue ) {
        String value = intValue + "";
        for ( int i : intArr ) {
            if ( value.equals( i + "" ) ) {
                return true;
            }
        }
        return false;
    }

    /**
     * @description: 检查create / drop index任务信息结果信息（通过索引名、集合、结果码获取任务）
     * @param taskTypeDesc
     *            任务类型
     * @param resultCode
     *            任务错误码 成功:0 失败:错误码
     * @param csName
     * @param clName
     * @param indexName
     *            检验测索引名
     */
    public static void checkIndexTaskResult( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName, int resultCode ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "IndexName", indexName );
        matcher.put( "ResultCode", resultCode );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        Assert.assertEquals( taskNum, 1, "index task num should be 1!" );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status );

        boolean isMainTask = taskInfo.containsField( "IsMainTask" );
        if ( !isMainTask ) {
            // 校验组信息
            @SuppressWarnings("unchecked")
            List< BSONObject > actGroupInfos = ( List< BSONObject > ) taskInfo
                    .get( "Groups" );
            List< String > groupNames = new ArrayList<>();
            for ( int i = 0; i < actGroupInfos.size(); i++ ) {
                BSONObject groupInfo = actGroupInfos.get( i );
                String groupName = ( String ) groupInfo.get( "GroupName" );
                groupNames.add( groupName );
                int groupResultCode = ( int ) groupInfo.get( "ResultCode" );
                Assert.assertEquals( groupResultCode, resultCode,
                        "check group task error! groupName=" + groupName );
            }

            List< String > expGroupNames = getCLGroupNames( db, csName,
                    clName );
            Assert.assertEquals( groupNames, expGroupNames,
                    "check group error!task=" + taskInfo.toString() );
        }

    }

    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName ) {
        checkIndexTask( db, taskTypeDesc, csName, clName, indexName, 0 );
    }

    /**
     * @description: 检查未创建任务
     * @param {Sequoiadb}
     *            db
     * @param {String}
     *            csName
     * @param {String}
     *            clName
     * @param {String}
     *            taskTypeDesc //任务类型
     * @return {*}
     */
    public static void checkNoTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        int taskNum = 0;
        BSONObject taskInfo = null;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        Assert.assertEquals( taskNum, 0,
                "check task should be no exist! act task =" + taskInfo );
        cursor.close();
    }

    public static boolean isExistTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        boolean isExist = false;

        while ( cursor.hasNext() ) {
            cursor.getNext();
            isExist = true;
        }
        cursor.close();
        return isExist;
    }

    /**
     * @description: 随机获取group一个节点
     * @param db
     * @param groupName
     * @return
     */
    // public static String getGroupOneNode( Sequoiadb db, String groupName ) {
    //
    // List< BasicBSONObject > nodeAddrs = new ArrayList<>();
    // nodeAddrs = CommLib.getGroupNodes( db, groupName );
    // Random random = new Random();
    // int serialNum = random.nextInt( nodeAddrs.size() );
    // String nodeName = nodeAddrs.get( serialNum ).getString( "hostName" )
    // + ":" + nodeAddrs.get( serialNum ).getString( "svcName" );
    // return nodeName;
    // }

    /**
     * @description: 随机获取CL的一个节点
     * @param db
     * @param csName
     * @param clName
     * @return
     */
    public static String getCLOneNode( Sequoiadb db, String csName,
            String clName ) {
        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        nodeAddrs = CommLib.getCLNodes( db, csName, clName );
        Random random = new Random();
        int serialNum = random.nextInt( nodeAddrs.size() );
        String nodeName = nodeAddrs.get( serialNum ).getString( "hostName" )
                + ":" + nodeAddrs.get( serialNum ).getString( "svcName" );
        System.out.println( "nodeAddrs  -- " + nodeAddrs.toString() );
        System.out.println( "hostName -- "
                + nodeAddrs.get( serialNum ).getString( "hostName" ) );
        System.out.println( "svcName -- "
                + nodeAddrs.get( serialNum ).getString( "svcName" ) );
        System.out.println( "units nodeName -- " + nodeName );
        return nodeName;
    }

    /**
     * @description: 随机获取CL的一个节点
     * @param db
     * @param csName
     * @param clName
     * @return
     */
    public static BasicBSONObject getCLOneNodeName( Sequoiadb db, String csName,
            String clName ) {
        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        nodeAddrs = CommLib.getCLNodes( db, csName, clName );
        Random random = new Random();
        int serialNum = random.nextInt( nodeAddrs.size() );
        return nodeAddrs.get( serialNum );
    }

    /**
     * @description: 检测节点上是否存在本地索引信息
     * @param db
     * @param csName
     * @param clName
     * @param indexName
     *            需要检测的索引名
     * @param indexNodeName
     *            需要检测的节点名
     * @param isExist
     *            是否存在索引
     */
    public static void checkStandaloneIndexOnNode( Sequoiadb db, String csName,
            String clName, String indexName, String indexNodeName,
            Boolean isExist ) {
        List< BasicBSONObject > nodes = CommLib.getCLNodes( db, csName,
                clName );
        for ( BasicBSONObject node : nodes ) {
            String nodeUrl = node.getString( "hostName" ) + ":"
                    + node.getString( "svcName" );
            try ( Sequoiadb data = new Sequoiadb( nodeUrl, "", "" )) {
                DBCollection dbcl = CommLib.getCL( data, csName, clName );
                if ( isExist ) {
                    if ( indexNodeName.equals( nodeUrl ) ) {
                        Assert.assertTrue( dbcl.isIndexExist( indexName ),
                                "nodeName -- " + nodeUrl );
                    } else {
                        Assert.assertFalse( dbcl.isIndexExist( indexName ),
                                "nodeName -- " + nodeUrl );
                    }
                } else {
                    if ( indexNodeName.equals( nodeUrl ) ) {
                        Assert.assertFalse( dbcl.isIndexExist( indexName ) );
                    }
                }
            }
        }
    }

    /**
     * @description: 检测节点上是否存在本地索引信息
     * @param db
     * @param csName
     * @param clName
     * @param indexName
     *            需要检测的索引名
     * @param indexNodeName
     *            需要检测的节点名
     * @param isExist
     *            是否存在索引
     */
    public static void checkStandaloneIndexOnNode( Sequoiadb db, String csName,
            String clName, String indexName, List< String > indexNodeName,
            Boolean isExist ) {
        List< BasicBSONObject > nodes = CommLib.getCLNodes( db, csName,
                clName );
        for ( BasicBSONObject node : nodes ) {
            String nodeUrl = node.getString( "hostName" ) + ":"
                    + node.getString( "svcName" );
            try ( Sequoiadb data = new Sequoiadb( nodeUrl, "", "" )) {
                DBCollection dbcl = CommLib.getCL( data, csName, clName );
                if ( isExist ) {
                    if ( indexNodeName.contains( nodeUrl ) ) {
                        Assert.assertTrue( dbcl.isIndexExist( indexName ) );
                    } else {
                        Assert.assertFalse( dbcl.isIndexExist( indexName ) );
                    }
                } else {
                    if ( indexNodeName.contains( nodeUrl ) ) {
                        Assert.assertFalse( dbcl.isIndexExist( indexName ) );
                    }
                }
            }
        }
    }

    /**
     * @description 检查独立索引任务
     * @param db
     * @param taskTypeDesc
     * @param csName
     * @param clName
     * @param nodeName
     * @param indexName
     */
    public static void checkStandaloneIndexTask( Sequoiadb db,
            String taskTypeDesc, String csName, String clName, String nodeName,
            String indexName ) {
        checkStandaloneIndexTask( db, taskTypeDesc, csName, clName, nodeName,
                indexName, 0 );
    }

    /**
     *
     * @param db
     * @param taskTypeDesc
     * @param csName
     * @param clName
     * @param nodeName
     * @param indexName
     * @param resultCode
     */
    public static void checkStandaloneIndexTask( Sequoiadb db,
            String taskTypeDesc, String csName, String clName, String nodeName,
            String indexName, int resultCode ) {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "NodeName", nodeName );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TASKS, matcher,
                null, null );

        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        Assert.assertEquals( taskNum, 1, "index task num should be 1!" );
        // 校验结果状态码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        Assert.assertEquals( actResultCode, resultCode );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status );

        // 校验索引名
        String actIndexName = ( String ) taskInfo.get( "IndexName" );
        Assert.assertEquals( actIndexName, indexName );

        // 校验节点信息
        String actNodeName = ( String ) taskInfo.get( "NodeName" );
        Assert.assertEquals( actNodeName, nodeName );

        // 校验独立索引
        BasicBSONObject indexDef = ( BasicBSONObject ) taskInfo
                .get( "IndexDef" );
        Boolean standalone = ( Boolean ) indexDef.get( "Standalone" );
        Assert.assertTrue( standalone );
    }

    /**
     * @description 检查create / drop 独立索引任务信息（索引名、组名、结果状态码比较）
     * @param db
     * @param taskTypeDesc
     *            任务类型
     * @param csName
     * @param clName
     * @param nodeName
     *            指定创建独立索引的节点名
     * @param indexName
     * @param resultCodes
     *            预期任务错误码，针对多个任务不同错误码校验可以指定多个错误码
     */
    public static void checkStandaloneIndexTask( Sequoiadb db,
            String taskTypeDesc, String csName, String clName, String nodeName,
            String indexName, int[] resultCodes, int taskNums ) {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "NodeName", nodeName );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TASKS, matcher,
                null, null );

        BSONObject taskInfo = null;
        int status = 9;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            // 校验结果状态码
            int actResultCode = ( int ) taskInfo.get( "ResultCode" );
            boolean isEqual = intArrayContainsField( resultCodes,
                    actResultCode );
            Assert.assertTrue( isEqual, "The act resultCode=" + actResultCode
                    + ",act resultCodes = " + resultCodes.toString() );

            int actStatus = ( int ) taskInfo.get( "Status" );
            Assert.assertEquals( actStatus, status );

            // 校验索引名
            String actIndexName = ( String ) taskInfo.get( "IndexName" );
            Assert.assertEquals( actIndexName, indexName );

            // 校验节点信息
            String actNodeName = ( String ) taskInfo.get( "NodeName" );
            Assert.assertEquals( actNodeName, nodeName );

            // 校验独立索引
            BasicBSONObject indexDef = ( BasicBSONObject ) taskInfo
                    .get( "IndexDef" );
            Boolean standalone = ( Boolean ) indexDef.get( "Standalone" );
            Assert.assertTrue( standalone );
            taskNum++;
        }
        cursor.close();

        Assert.assertEquals( taskNum, taskNums, "index task num should be 1!" );

    }

    /**
     * @description 检查节点上是否存在独立索引任务
     * @param db
     * @param taskTypeDesc
     *            任务类型
     * @param csName
     * @param clName
     * @param nodeName
     *            指定创建独立索引的节点名
     * @param indexName
     */
    public static void checkNoIndexStandaloneTask( Sequoiadb db,
            String taskTypeDesc, String csName, String clName, String nodeName,
            String indexName ) {

        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "NodeName", nodeName );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TASKS, matcher,
                null, null );

        int taskNum = 0;
        BSONObject taskInfo = null;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();
        Assert.assertEquals( taskNum, 0,
                "check task should be no exist! act task =" + taskInfo );
    }

    /**
     * @description: 通过索引快照查看是否存在本地索引
     * @param db
     * @param csName
     * @param clName
     * @param indexName
     * @param indexNodeName
     *            需要检测的节点
     * @param isExist
     *            节点上是否存在独立索引
     */
    public static void checkStandaloneIndexBySnap( Sequoiadb db, String csName,
            String clName, String indexName, String indexNodeName,
            Boolean isExist ) {
        BSONObject indexInfo = new BasicBSONObject();
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        DBCursor cursor = dbcl.snapshotIndexes(
                new BasicBSONObject( "IndexDef.name", indexName )
                        .append( "Nodes.NodeName", indexNodeName ),
                null, null, null, 0, -1 );
        if ( isExist ) {
            Assert.assertTrue( cursor.hasNext() );
            while ( cursor.hasNext() ) {
                indexInfo = cursor.getNext();
            }
            cursor.close();
            List< BasicBSONObject > nodes = ( List< BasicBSONObject > ) indexInfo
                    .get( "Nodes" );
            Assert.assertEquals( nodes.size(), 1,
                    "Inconsistent number of nodes" + nodes.toString() );
            String nodeName = nodes.get( 0 ).getString( "NodeName" );
            Assert.assertEquals( indexNodeName, nodeName, "expected node is "
                    + indexNodeName + ", actual node is " + nodeName );
        } else {
            Assert.assertFalse( cursor.hasNext() );
        }
    }

    /**
     * @description: 通过索引快照查看是否存在本地索引
     * @param db
     * @param csName
     * @param clName
     * @param indexName
     * @param indexNodeName
     *            需要检测的节点List< String >
     * @param isExist
     *            节点上是否存在独立索引
     */
    public static void checkStandaloneIndexBySnap( Sequoiadb db, String csName,
            String clName, String indexName, List< String > indexNodeName,
            Boolean isExist ) {
        BSONObject indexInfo = new BasicBSONObject();
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        DBCursor cursor = dbcl.snapshotIndexes(
                new BasicBSONObject( "IndexDef.name", indexName ), null, null,
                null, 0, -1 );
        if ( isExist ) {
            Assert.assertTrue( cursor.hasNext() );
            while ( cursor.hasNext() ) {
                indexInfo = cursor.getNext();
            }
            cursor.close();
            List< BasicBSONObject > nodes = ( List< BasicBSONObject > ) indexInfo
                    .get( "Nodes" );
            for ( BasicBSONObject node : nodes ) {
                String nodeName = node.getString( "NodeName" );
                Assert.assertTrue( indexNodeName.contains( nodeName ),
                        "node " + nodeName + " have index" );
                indexNodeName.remove( nodeName );
            }
            Assert.assertEquals( indexNodeName.size(), 0,
                    "node " + indexNodeName.toString() + " not have index" );
        } else {
            if ( cursor.hasNext() ) {
                while ( cursor.hasNext() ) {
                    indexInfo = cursor.getNext();
                }
                cursor.close();
                List< BasicBSONObject > nodes = ( List< BasicBSONObject > ) indexInfo
                        .get( "Nodes" );
                for ( BasicBSONObject node : nodes ) {
                    String nodeName = node.getString( "NodeName" );
                    Assert.assertFalse( indexNodeName.contains( nodeName ),
                            "node " + nodeName + " have index" );
                }
            }
        }
    }

}
