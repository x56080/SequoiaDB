/******************************************************************************
 * @Description   : seqDB-31761:超过MaxKeepTime节点未恢复，自动停止Critical模式
 * @Author        : tangtao
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2023.10.31
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31761";
testConf.clOpt = { ReplSize: 0 };

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
      println( new Date() + ": Begin to stop slave nodes" ) ;
      // 停止group中备节点，让节点停止之后不启动
      stopNodes( db, rg, slaveNodes );

      // 剩余一个节点启动Critical模式
      var minKeepTime = 1;
      var maxKeepTime = 2;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      
      println( new Date() + ": Begin to startCriticalMode" ) ;
      rg.startCriticalMode( options );
      println( new Date() + ": End to startCriticalMode" ) ;

      var beginTime = new Date();

      // 等待超过MinKeepTime
      var waitTime = minKeepTime + 0.2;
      println( new Date() + ": Begin to wait min time" ) ;
      validateWaitTime( beginTime, waitTime );
      var properties = { NodeName: masterNodeName };
      println( new Date() + ": Begin to checkStartCriticalMode when wait min time" ) ;
      checkStartCriticalMode( db, srcGroup, properties );

      // 等待超过MaxKeepTime
      var waitTime = maxKeepTime + 0.2;
      println( new Date() + ": Begin to wait max time" ) ;
      validateWaitTime( beginTime, waitTime );
      println( new Date() + ": Begin to checkStartCriticalMode when wait max time" ) ;
      checkStopCriticalMode( db, srcGroup );

      println( new Date() + ": Begin to insert data" ) ;
      // 插入数据报错
      assert.tryThrow( SDB_CLS_NOT_PRIMARY, function()
      {
         dbcl.insert( { a: 1 } );
      } );

      println( new Date() + ": Begin to start slave nodes" ) ;
      // 启动节点
      startNodes( db, rg, slaveNodes );
      // 等待LSN一致
      commCheckBusinessStatus( db );
   }
   finally
   {
      rg.start();
      commCheckBusinessStatus( db );
      db.updateConf( { sharingbreak: actSharingbreak } );
   }
}