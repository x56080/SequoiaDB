/******************************************************************************
 * @Description   : seqDB-34019:启动Critical模式的节点中存在异常节点
 * @Author        : liuli
 * @CreateTime    : 2024.03.01
 * @LastEditTime  : 2024.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true; 

main( test );
function test ()
{
   var location = "location_34019";
   var srcGroup = commGetDataGroupNames( db )[0];

   // 获取group中的所有节点
   var nodes = commGetGroupNodes( db, srcGroup );

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

   // srcGroup所有节点设置Location
   var rg = db.getRG( srcGroup );
   for( var i = 0; i < nodes.length; i++ )
   {
      var node = rg.getNode( nodes[i]["HostName"], nodes[i]["svcname"] );
      node.setLocation( location );
   }

   try
   {
      // 停一个备节点
      var slaveNodeName = slaveNodes[0];
      var slaveNode = rg.getNode( slaveNodeName );
      slaveNode.stop();

      // 指定停掉的节点启动Critical模式
      var options = { NodeName: slaveNodeName, MinKeepTime: 5, MaxKeepTime: 10 };
      assert.tryThrow( SDB_NET_CANNOT_CONNECT, function()
      {
         rg.startCriticalMode( options );
      } );

      if ( nodes.length >= 3 )
      {
          // 指定Location启动Critical模式，Location中有异常节点
          var options = { Location: location, MinKeepTime: 5, MaxKeepTime: 10 };
          rg.startCriticalMode( options );
          checkStartCriticalMode( db, srcGroup, options );
          rg.stopCriticalMode();

          // 再次停一个备节点
          var slaveNode1 = rg.getNode( slaveNodes[1] );
          slaveNode1.stop();

          // 指定Location启动Critical模式，Location中异常节点超过半数
          var options = { Location: location, MinKeepTime: 5, MaxKeepTime: 10 };
          if ( nodes.length <= 4 )
          {
              assert.tryThrow( SDB_NET_CANNOT_CONNECT, function()
              {
                 rg.startCriticalMode( options );
              } );
          }
          else
          {
              rg.startCriticalMode( options );
              checkStartCriticalMode( db, srcGroup, options );
              rg.stopCriticalMode();
          }
      }
   } finally
   {
      rg.start();
      rg.stopCriticalMode();
      commCheckBusinessStatus( db );
      for( var i = 0; i < nodes.length; i++ )
      {
         var node = rg.getNode( nodes[i]["HostName"], nodes[i]["svcname"] );
         node.setLocation( "" );
      }
   }
}