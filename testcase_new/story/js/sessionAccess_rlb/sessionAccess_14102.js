/* *****************************************************************************
@description: seqDB-14102:设置会话访问属性，指定preferedinstance值instanceid所在节点为主/备，同时指定S/M 
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

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
   var clName = CHANGEDPREFIX + "_14102";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName });
   insertData( cl );

   var instanceid = [30, 31]
   var slavePos1 = primaryPos === 1 ? 2 : 1;
   var slavePos2 = primaryPos === 3 ? 2 : 3;
   var position = [ primaryPos, slavePos1 ];
   for( var i = 0; i < instanceid.length; i++ )
   {
      var hostName = group[ position[i] ].HostName;
      var svcName = group[ position[i] ].svcname;
      updateConf( db, { instanceid: instanceid[i] }, { NodeName: hostName + ":" + svcName }, -264 );
   }
   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();
   try
   {
      commCheckBusinessStatus( db );
      db.invalidateCache();
  
      var options = { PreferedInstance: [30, "S"] };
      var expAccessNodes = [ group[ primaryPos ].HostName + ":" + group[ primaryPos ].svcname ];
      checkAccessNodes( cl, expAccessNodes, options );
  
      options = { PreferedInstance: [31, "M"] };
      var expAccessNodes = [ group[ slavePos1 ].HostName + ":" + group[ slavePos1 ].svcname ];
      checkAccessNodes( cl, expAccessNodes, options );
      commDropCL( db, COMMCSNAME, clName, false, false ) ;
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: groupName }, -264 );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
   }
}

