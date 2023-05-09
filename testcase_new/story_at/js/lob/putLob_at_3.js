/***************************************************************************************************
 * @Description: 普通表、分区表 putLob 不同数据量测试
 * @ATCaseID: putLob_at_3
 * @Author: Yang Qincheng
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 05/05/2023 Yang Qincheng  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：普通复制组
 * 测试场景：
 *    向普通表、分区表中分别 put 小于 lobPageSize - 1KB 的数据、大于 lobPageSize - 1KB 的数据
 * 测试步骤：
 *    1. 准备 1KB、4KB 的 lob 数据
 *    2. 获取数据库快照的数据
 *    3. 创建普通表/分区表，lobPageSize 为 4KB
 *    4. 调用 putLobFile() 写入 1KB 的 lob 数据
 *    5. 读取并检查写入的 lob 数据
 *    6. 执行 cl 快照、当前会话快照，检查 TotalLobs、TotalValidLobSize、TotalLobPut 等 lob 监控指标
 *    7. 重复执行 3-6 步骤，写入 4KB 数据
 *    8. 再次获取数据库快照的数据
 *    9. 检查两次数据库快照的数据是否正确
 * 期望结果：
 *    步骤 4：执行成功
 *    步骤 5：lob 数据正确
 *    步骤 6：lob 各项监控指标正常
 *    步骤 9：数据库快照结果正确
 **************************************************************************************************/

 testConf.skipStandAlone = true;

 main(test);
 function test() {
    var csName = "put_lob_at_3_cs";
    var clName = "put_lob_at_3_cl";
    var data1KBFile = "put_lob_at_3_data1KB.txt";
    var data4KBFile = "put_lob_at_3_data4KB.txt";
    var readFile = "put_lob_at_3_read.txt";

    var len1KB = 1024;
    var len4KB = 4 * 1024;
    var data1KB = generateLobData( len1KB );
    var data4KB = generateLobData( len4KB );
    generateLobFile( data1KBFile, data1KB );
    generateLobFile( data4KBFile, data4KB );

    var groups = commGetGroups( db );
    var beforeObj = getDBSnapshot( db );

    // case 1: 普通表
    putLobToCL( csName, clName, groups, data1KBFile, readFile );
    putLobToCL( csName, clName, groups, data4KBFile, readFile );

    // case 2: 分区表
    putLobToSplitCL( csName, clName, groups, data1KBFile, readFile );
    putLobToSplitCL( csName, clName, groups, data4KBFile, readFile );

    var afterObj = getDBSnapshot( db );
    var lobSize = ( len1KB + len4KB ) * 2;
    checkDBSnapshotData( beforeObj, afterObj, 4, lobSize );

    removeLobFile( data1KBFile );
    removeLobFile( data4KBFile );
    commDropCS( db, csName, true, "drop env" );
 }

 function putLobToCL( csName, clName, groups, dataFile, readFile ) {
    var conn = new Sdb( COORDHOSTNAME, COORDSVCNAME );
    try {
        var groupName = groups[0][0]["GroupName"]
        var cl = prepareCL( conn, csName, clName, groupName );

        var nodeName = getMasterNodeName( conn, groupName );
        var nodeNameArr = new Array();
        nodeNameArr.push( nodeName );

        putAndcheckLob( cl, undefined, dataFile, readFile );
        checkLobInCLSnap( conn, nodeNameArr, csName + "." + clName, 1, dataFile );
        checkLobInCurSessSnap( conn, nodeNameArr, 1, dataFile );
    } finally {
        conn.close();
    }
 }

 function prepareCL( conn, csName, clName, groupName ) {
    commDropCS( conn, csName, true, "init env" );
    var cl = conn.createCS( csName, { "LobPageSize": 4 * 1024 } ).createCL( clName, { "ReplSize": 0, "Group": groupName } );
    return cl;
 }

 function putLobToSplitCL( csName, clName, groups, dataFile, readFile ) {
    var conn = new Sdb( COORDHOSTNAME, COORDSVCNAME );
    try {
        var srcGroup = groups[0][0]["GroupName"];
        var dstGroup = groups[1][0]["GroupName"];
        var cl = prepareSplitCL( conn, csName, clName, srcGroup, dstGroup );

        var srcNodeName = getMasterNodeName( conn, srcGroup );
        var dstNodeName = getMasterNodeName( conn, dstGroup );
        var nodeNameArr = new Array();
        nodeNameArr.push( srcNodeName );
        nodeNameArr.push( dstNodeName );

        putAndcheckLob( cl, undefined, dataFile, readFile );
        checkLobInCLSnap( conn, nodeNameArr, csName + "." + clName, 1, dataFile );

        checkLobInCurSessSnap( conn, nodeNameArr, 1, dataFile );
    } finally {
        conn.close();
    }
 }

 function prepareSplitCL( conn, csName, clName, srcGroup, dstGroup ) {
    commDropCS( conn, csName, true, "init env" );

    var clOpt = { "Group": srcGroup, "ShardingType": "hash", "ShardingKey": { a: 1 }, "ReplSize": 0 }
    var cl = conn.createCS( csName, { "LobPageSize": 4 * 1024 } ).createCL( clName, clOpt );
    insertDoc( cl, 1000 );
    cl.split( srcGroup, dstGroup, 50 );
    return cl;
 }

 function getDBSnapshot( db ) {
    var sel = { "TotalLobPut": "",
                "TotalLobWriteSize": "" }
    var cursor = db.snapshot( SDB_SNAP_DATABASE, {}, sel );
    var obj = cursor.next().toObj();
    println(cursor.current());
    cursor.close();

    return obj;
 }

 function checkDBSnapshotData( beforeObj, afterObj, lobNum, lobSize ) {
    var totalLobPut = afterObj["TotalLobPut"] - beforeObj["TotalLobPut"];
    var totalLobWriteSize = afterObj["TotalLobWriteSize"] - beforeObj["TotalLobWriteSize"];

    // 因为协调节点聚合数据时，会将协调节点、数据节点的监控数据累加，因此聚合后，数据量会翻倍 
    assert.equal( totalLobPut, lobNum * 2 );
    assert.equal( totalLobWriteSize, lobSize * 2 );
 }
