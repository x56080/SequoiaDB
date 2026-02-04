/******************************************************************************
 * @Description   : seqDB-31754:数据节点2副本异常，剩余节点设置 Critical 模式
 * @Author        : liuli
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2024.01.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31754";

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;
   var dbcl = args.testCL;

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

   // 获取sharingbreak
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { GroupName: srcGroup }, { sharingbreak: 1 } );
   var actSharingbreak = cursor.current().toObj().sharingbreak;
   println( "actSharingbreak: " + actSharingbreak );

   // 修改sharingbreak为默认值
   db.deleteConf( { sharingbreak: 1 } );

   // 获取主节点
   var rg = db.getRG( srcGroup );
   var masterNode = rg.getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   try
   {
      killNodes( db, rg, slaveNodes );

      // 剩余一个节点启动Critical模式
      var minKeepTime = 10;
      var maxKeepTime = 20;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );

      // 插入数据并校验
      var docs = insertBulkData( dbcl, 1000 );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );

      // 检查Critical模式
      checkStartCriticalMode( db, srcGroup, options );

      // 启动节点
      startNodes( db, rg, slaveNodes );

      // 等待LSN一致
      commCheckBusinessStatus( db );

      // 停止Critical模式
      rg.stopCriticalMode();

      // 检查Critical模式停止
      checkStopCriticalMode( db, srcGroup );
   }
   finally
   {
      rg.start() ;
      commCheckBusinessStatus( db );
      rg.stopCriticalMode() ;
      // 恢复actSharingbreak配置
      db.updateConf( { sharingbreak: actSharingbreak } );
   }
}