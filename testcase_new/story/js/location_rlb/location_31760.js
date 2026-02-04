/******************************************************************************
 * @Description   : seqDB-31760:超过MinKeepTime节点未恢复，手动停止Critical模式
 * @Author        : liuli
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2024.03.11
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31760";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;
   var dbcl = args.testCL;

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

   // 获取主节点
   var rg = db.getRG( srcGroup );
   var masterNode = rg.getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   try
   {
      // 停止所有备节点
      stopNodes( db, rg, slaveNodes ) ;

      // 剩余一个节点启动Critical模式
      var minKeepTime = 1;
      var maxKeepTime = 20;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );

      var beginTime = new Date();

      // 插入数据并校验
      var docs = insertBulkData( dbcl, 1000 );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );

      // 等待超过MinKeepTime
      var waitTime = minKeepTime + 0.2;
      validateWaitTime( beginTime, waitTime );

      // 停止Critical模式
      rg.stopCriticalMode();

      // 检查Critical模式已经停止
      checkStopCriticalMode( db, srcGroup );

      // 插入数据报错
      assert.tryThrow( [SDB_CLS_NODE_NOT_ENOUGH, SDB_CLS_NOT_PRIMARY], function()
      {
         dbcl.insert( { a: 1 } );
      } );

      // 启动所有备节点
      startNodes( db, rg, slaveNodes ) ;

      // 等待LSN一致
      commCheckBusinessStatus( db );
   }
   finally
   {
      rg.start() ;
      rg.stopCriticalMode() ;
      commCheckBusinessStatus( db );
   }
}