/******************************************************************************
 * @Description   : seqDB-31759:超过MinKeepTime后节点恢复，自动停止 Critical 模式
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.06.08
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31759";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var clName = "cl_31759";
   var srcGroup = args.srcGroupName;
   var dbcl1 = args.testCL;

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

   // 获取主节点
   var rg = db.getRG( srcGroup );
   var masterNode = rg.getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   try
   {
      stopNodes( db, rg, slaveNodes );

      // 剩余一个节点启动Critical模式
      var minKeepTime = 1;
      var maxKeepTime = 2;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );

      // 开启事务 
      db.transBegin();

      // 插入数据并校验
      var docs = insertBulkData( dbcl1, 1000 );
      var cursor = dbcl1.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );

      // 提交事务
      db.transCommit();

      var beginTime = new Date();

      // 等待超过MaxKeepTime
      var waitTime = maxKeepTime + 0.2;
      validateWaitTime( beginTime, waitTime );
      checkStopCriticalMode( db, srcGroup );

      // 启动所有备节点
      startNodes( db, rg, slaveNodes );
      // 等待LSN一致
      commCheckBusinessStatus( db );

      var dbcl2 = commCreateCL( db, COMMCSNAME, clName, { Group: srcGroup } );
      var docs = insertBulkData( dbcl2, 1000 );
      var cursor = dbcl2.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
   }
   finally
   {
      rg.start() ;
      rg.stopCriticalMode();
      commCheckBusinessStatus( db );
   }
}
