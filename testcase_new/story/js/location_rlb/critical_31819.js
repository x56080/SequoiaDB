/******************************************************************************
 * @Description   : seqDB-31819:启动Critical模式同时指定NodeName和Location
 * @Author        : tangtao
 * @CreateTime    : 2023.06.05
 * @LastEditTime  : 2023.06.05
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31819";

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;
   var dbcl = args.testCL;

   var location1 = "guangzhou_31819";
   var location2 = "shenzhen_31819";

   // 获取group中的主备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

   // 获取主节点
   var rg = db.getRG( srcGroup );
   var masterNode = rg.getMaster();
   var masterNodeID = masterNode.getNodeDetail().split( ":" )[0];
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   var salveNodeID1 = slaveNode1.getNodeDetail().split( ":" )[0];
   var salveNodeID2 = slaveNode2.getNodeDetail().split( ":" )[0];

   // 节点设置Location
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location2 );
   try
   {
      // 主节点启动Critical模式并检查Critical模式
      var options = { NodeName: masterNodeName, Location: location1,
                      MinKeepTime: 5, MaxKeepTime: 15 };
      rg.startCriticalMode( options );

      var properties1 = { NodeID: masterNodeID };
      checkStartCriticalMode( db, srcGroup, properties1 );

      // 恢复环境
      rg.stopCriticalMode();

      // 备节点启动Critical模式并检查Critical模式
      var options = { NodeName: slaveNodes[1], Location: location1,
                      MinKeepTime: 5, MaxKeepTime: 15 };
      rg.startCriticalMode( options );

      var properties2 = { NodeID: salveNodeID2 };
      checkStartCriticalMode( db, srcGroup, properties2 );

      // 恢复环境
      rg.stopCriticalMode();

      masterNode.setLocation( "" );
      slaveNode1.setLocation( "" );
      slaveNode2.setLocation( "" );
   }
   finally
   {
      rg.start();
      commCheckBusinessStatus( db );
   }
}