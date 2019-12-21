/* *****************************************************************************
@discretion: seqDB-20181:指定其他组的备节点ID/本组故障的备节点ID，重新选主
@author：2018-11-04 zhao xiaoni
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test()
{
   var groups = getGroupsWithNodeNum( 3 ); 
   if( groups.length === 0 )
   {
      return;
   }
   var group1 = groups[0];
   var group2 = groups[1]
   var groupName1 = group1[0].GroupName;
   var groupName2 = group2[0].GroupName;
   var primaryPos1 = group1[0].PrimaryPos;
   var primaryPos2 = group2[0].PrimaryPos;
   var slaveNode1Pos = primaryPos1 === 1 ? primaryPos1 + 1 : 1;
   var slaveNode1 = group1[ slaveNode1Pos ];
   var masterNode = group1[ primaryPos1 ];
   var slaveNode2Pos = primaryPos2 === 1 ? primaryPos2 + 1 : 1;   
   var slaveNode2 = group2[ slaveNode2Pos ];

   waitSync(masterNode, slaveNode1);
      
   //指定其他组的备节点ID，重新选主
   var nodeID = slaveNode2.NodeID;
   println("start to reelect node " + nodeID + " to primary node");
   try
   {
      db.getRG(groupName1).reelect({Seconds: 60, NodeID: parseInt(nodeID)});
      throw "Reelect should be failed!";
   } 
   catch(e)
   {
      if(e !== -155)
      {
         throw new Error(e);
      }
   }   

   //指定本组故障的备节点ID，重新选主
   db.getRG(groupName1).getNode(slaveNode1.HostName, slaveNode1.svcname).stop();
   try
   {
      nodeID = slaveNode1.NodeID;
      println("start to reelect node " + nodeID + " to primary node");
      db.getRG( groupName1 ).reelect({Seconds: 60, NodeID: parseInt(nodeID)});
      
      var node = db.getRG( groupName1 ).getMaster();
      var nodeName = node.getHostName() + ":" + node.getServiceName();
      if( nodeName === masterNode.HostName + ":" + masterNode.svcname )
      {
         throw new Error( "The master node should not be " + nodeName );
      }
   }
   finally
   {
      db.getRG(groupName1).getNode(slaveNode1.HostName, slaveNode1.svcname).start();
   }
 
   //指定主机名和服务名不存在，重新选主
   var hostName = slaveNode2.HostName;
   var svcName = slaveNode2.svcname;
   println("start to reelect node with hostName " + hostName + " and serviceName " + svcName + " to primary node");
   try
   {
      db.getRG(groupName1).reelect({Seconds: 60, HostName: hostName, ServiceName: svcName});
      throw "Reelect should be failed!";
   }
   catch(e)
   {
      if(e !== -155)
      {
         throw new Error(e);
      }
   }
}
