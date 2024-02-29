/******************************************************************************
 * @Description   : seqDB-34018:启动Critical模式的节点包含主节点
 * @Author        : liuli
 * @CreateTime    : 2024.02.29
 * @LastEditTime  : 2024.02.29
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( args )
{
   var location = "location_34018";
   var srcGroup = commGetDataGroupNames( db )[0];

   // 获取group中的所有节点
   var nodes = commGetGroupNodes( db, srcGroup );

   // 获取主节点
   var rg = db.getRG( srcGroup );
   var masterNode = rg.getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   // srcGroup所有节点设置Location
   for( var i = 0; i < nodes.length; i++ )
   {
      var node = rg.getNode( nodes[i]["HostName"], nodes[i]["svcname"] );
      node.setLocation( location );
   }

   // 指定Location启动Critical模式，循环执行10次
   var options = { Location: location, MinKeepTime: 5, MaxKeepTime: 10 };
   var startCriticalNum = 10;
   for( var i = 0; i < startCriticalNum; i++ )
   {
      rg.startCriticalMode( options );
      // 预期主节点不变
      var masterNodeNew = rg.getMaster();
      var masterNodeNameNew = masterNodeNew.getHostName() + ":" + masterNodeNew.getServiceName();
      assert.equal( masterNodeName, masterNodeNameNew, "主节点变化" );
      rg.stopCriticalMode();
   }

   for( var i = 0; i < nodes.length; i++ )
   {
      var node = rg.getNode( nodes[i]["HostName"], nodes[i]["svcname"] );
      node.setLocation( "" );
   }
}