/******************************************************************************
 * @Description   : seqDB-27846:目标组无lob，源组100%切分至目标组 
 * @Author        : liuli
 * @CreateTime    : 2022.09.25
 * @LastEditTime  : 2022.10.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27852";
testConf.csOpt = { LobPageSize: 32768 };
testConf.clName = COMMCLNAME + "_27852";
testConf.clOpt = { ReplSize: 0, ShardingKey: { a: 1 } };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

// 源组和目标组节点数量不一致时会导致结果无法预估，先屏蔽用例
// main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob27852/";
   var fileName = "filelob_27852";
   var fileSize = 1024 * 50;

   // putLob占多个大对象页
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   dbcl.putLob( filePath + fileName );

   // 获取cl所在group的主节点，部分操作只在主节点统计
   var groupName = testPara.srcGroupName;
   var dstGroupNames = testPara.dstGroupNames;

   // 获取数据库快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   var dataBastInfo = getSnapshotLobStat( cursor );

   // 获取数据库快照信息，非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true } );
   var dataBastInfoRawData = getSnapshotLobStat( cursor );

   // 获取集合空间快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   var csInfo = getSnapshotLobStat( cursor );

   // 执行100%切分
   dbcl.split( groupName, dstGroupNames[0], 100 );

   // 查看数据库快照并校验结果，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo );

   // 查看快照进行排序，此前的预期结果已进行过排序
   var option = new SdbSnapshotOption().cond( { RawData: true } ).sort( { NodeName: 1 } );
   var cursor = db.snapshot( SDB_SNAP_DATABASE, option );
   checkSnapshot( cursor, dataBastInfoRawData );

   // 查看集合空间快照并校验结果，聚合结果
   csInfo[0]["TotalLobPut"] = 0;
   csInfo[0]["TotalLobAddressing"] = 0;
   csInfo[0]["TotalLobWriteSize"] = 0;
   csInfo[0]["TotalLobWrite"] = 0;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo );

   deleteTmpFile( filePath );
}