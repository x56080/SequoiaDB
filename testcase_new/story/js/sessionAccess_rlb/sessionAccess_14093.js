/* *****************************************************************************
@description: seqDB-14093:设置会话访问属性，指定preferedinstance为instanceid存在的节点和[M/S/A]
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

//SEQUOIADBMAINSTREAM-5283，待开发修改问题单后此用例需要整体进行优化及调试
//main( test );

function test()
{
   var groups = getGroupsWithNodeNum( 3 );
   if( groups.length === 0 )
   {
      return;
   }
   var group = groups[0];
   var groupName = group[0].GroupName;
   var primaryPos = group[0].PrimaryPos;
   var clName = CHANGEDPREFIX + "_14093";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName });
   insertData( cl );
   var instanceid = [ 30, 124, 8 ];
   for( var i = 0; i < instanceid.length; i++ )
   {
      var hostName = group[i+1].HostName;
      var svcName = group[i+1].svcname;
      updateConf( db, { instanceid: instanceid[i] }, { NodeName: hostName + ":" + svcName }, -264 );
   }
   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();
   commCheckBusinessStatus( db );
   db.invalidateCache();
   try
   {
      instanceid  = [124, 8, 30, "M"];
      actAccessNodes = [];
      for( var j = 0; j < 10; j++ )
      {
         db.setSessionAttr({ PreferedInstance: instanceid });
         var actAccessNode = cl.find().explain().current().toObj().NodeName;
         println("actAccessNode:::::"+actAccessNode);
         if( actAccessNodes.indexOf( actAccessNode ) === -1 )
         {
            actAccessNodes.push( actAccessNode );
         }
      }
      var expAccessNodes = [ group[ primaryPos ].HostName + ":" + group[ primaryPos ].svcname ];
    println("expAccessNodes1:"+JSON.stringify(expAccessNodes)+"\nactAccessNodes:"+JSON.stringify(actAccessNodes));
      checkAccessNodes( expAccessNodes, actAccessNodes );

      var actAccessNodes = [];
      instanceid = [124, 8, 30, "m"];
      db.setSessionAttr({ PreferedInstance: instanceid });
      expAccessNodes = getGroupNodes( groupName );
      for( var i = 0; i < 10; i++ )
      {
        db.setSessionAttr({ PreferedInstance: instanceid });
         actAccessNode = cl.find().explain().current().toObj().NodeName;
        println("actAccessNode:::::"+actAccessNode);
         if( actAccessNodes.indexOf( actAccessNode ) === -1 )
         {
            actAccessNodes.push( actAccessNode );
         }
      }
      println("expAccessNodes2:"+JSON.stringify(expAccessNodes)+"\nactAccessNodes:"+JSON.stringify(actAccessNodes));
      checkAccessNodes( expAccessNodes, actAccessNodes );

      instanceid = [124, 8, 30, "S"];
      expAccessNodes = [ group[1].HostName + ":" + group[1].svcname ];
      actAccessNode = cl.find().explain().current().toObj().NodeName;
      actAccessNodes = [ actAccessNode ];
println("expAccessNodes3:"+JSON.stringify(expAccessNodes)+"\nactAccessNodes:"+JSON.stringify(actAccessNodes));
      checkAccessNodes( expAccessNodes, actAccessNodes );

      instanceid = [124, 8, 30, "s"];
      expAccessNodes = [ group[1].HostName + ":" + group[1].svcname ];
      actAccessNode = cl.find().explain().current().toObj().NodeName;
      actAccessNodes = [ actAccessNode ];
println("expAccessNodes4:"+JSON.stringify(expAccessNodes)+"\nactAccessNodes:"+JSON.stringify(actAccessNodes));
      checkAccessNodes( expAccessNodes, actAccessNodes );
      instanceid = [124, 8, 30, "A"];
      expAccessNodes = [ group[1].HostName + ":" + group[1].svcname ];
      actAccessNode = cl.find().explain().current().toObj().NodeName;
      actAccessNodes = [ actAccessNode ];
println("expAccessNodes5:"+JSON.stringify(expAccessNodes)+"\nactAccessNodes:"+JSON.stringify(actAccessNodes));
      checkAccessNodes( expAccessNodes, actAccessNodes );

      instanceid = [124, 8, 30, "a"];
      expAccessNodes = [ group[1].HostName + ":" + group[1].svcname ];
      actAccessNode = cl.find().explain().current().toObj().NodeName;
      actAccessNodes = [ actAccessNode ];
println("expAccessNodes6:"+JSON.stringify(expAccessNodes)+"\nactAccessNodes:"+JSON.stringify(actAccessNodes));
      checkAccessNodes( expAccessNodes, actAccessNodes );
     instanceid = [124, 8, 30, "M", "S", "A"];
      expAccessNodes = [ group[ primaryPos ].HostName + ":" + group[ primaryPos ].svcname ];
      actAccessNode = cl.find().explain().current().toObj().NodeName;
      actAccessNodes = [ actAccessNode ];
      println("expAccessNodes7:"+JSON.stringify(expAccessNodes)+"\nactAccessNodes:"+JSON.stringify(actAccessNodes));
      checkAccessNodes( expAccessNodes, actAccessNodes );
     instanceid = [124, 8, 30, "A", "M", "S"];
      actAccessNodes = [];
      for( var j = 0; j < 20; j++ )
      {
         db.setSessionAttr({ PreferedInstance: instanceid });
         var actAccessNode = cl.find().explain().current().toObj().NodeName;
         if( actAccessNodes.indexOf( actAccessNode ) === -1 )
         {
            actAccessNodes.push( actAccessNode );
         }
      }
      expAccessNodes = getGroupNodes( groupName );
println("expAccessNodes8:"+JSON.stringify(expAccessNodes)+"\nactAccessNodes:"+JSON.stringify(actAccessNodes));
      checkAccessNodes( expAccessNodes, actAccessNodes );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: groupName }, -264 );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
   }
   commDropCL( db, COMMCSNAME, clName, false, false ) ;
}


