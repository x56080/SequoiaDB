/* *****************************************************************************
@description: seqDB-14085:设置会话属性，preferedinstance指定instanceid与其它节点下标相同的实例 
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

//SEQUOIADBMAINSTREAM-5283
//与用例14101相似，与开发确认当节点的instanceid更新成与另外一个节点的下标相同时，访问的节点从两个节点中随机选择，但实际结果是大多数
//情况下只访问了被设置instanceid值的节点，不确定是否是上面问题单中的问题导致的，待问题单解决后再放开此用例
//main( test );
function test()
{
   var clName = CHANGEDPREFIX + "_14085";
   var groups = commGetGroups( db );
   var group = groups[0].sort( sortBy( "NodeID" ) );
   var groupName = group[0].GroupName;
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var instanceid = 2;
   var hostName = group[1].HostName;
   var svcName = group[1].svcname;
   var expAccessNodes = [ hostName + ":" + svcName ];
   updateConf ( db, { instanceid: instanceid }, { NodeName: hostName + ":" + svcName }, -264 );
   db.getRG( groupName ).getNode( hostName, svcName ).stop();
   db.getRG( groupName ).getNode( hostName, svcName ).start();
   try
   {
      commCheckBusinessStatus( db );
      db.invalidateCache();
      expAccessNodes.push( group[2].HostName + ":" + group[2].svcname);
      var options = { PreferedInstance: instanceid };
      checkAccessNodes( cl, expAccessNodes, options );
   }
   finally
   {
      deleteConf ( db, { instanceid: 1 }, {NodeName: hostName + ":" + svcName}, -264 );   
      db.getRG( groupName ).getNode( hostName, svcName ).stop();
      db.getRG( groupName ).getNode( hostName, svcName ).start();
      commCheckBusinessStatus( db );
   }
   commDropCL( db, COMMCSNAME, clName, false, false );
}

