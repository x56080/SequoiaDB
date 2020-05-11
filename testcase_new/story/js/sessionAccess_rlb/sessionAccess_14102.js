/* *****************************************************************************
@description: seqDB-14102:设置会话访问属性，指定preferedinstance值instanceid所在节点为主/备，同时指定S/M 
@author: 2020-5-11 Zhao Xiaoni  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test()
{
   var nodeNum = 3;
   var groupName = "rg_14102";
   var clName = CHANGEDPREFIX + "_14102";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeInfos = commCreateRG( db, groupName, nodeNum, hostName );
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 });
   insertData( cl );
  
   var expAccessNodes = [ nodeInfos[2].hostname + ":" + nodeInfos[2].svcname ];
   var options = { PreferedInstance: [ 3, "S" ] };
   checkAccessNodes( cl, expAccessNodes, options );
  
   expAccessNodes = [ nodeInfos[0].hostname + ":" + nodeInfos[0].svcname ];
   options = { PreferedInstance: [ 1, "M" ] };
   checkAccessNodes( cl, expAccessNodes, options );
 
   commDropCL( db, COMMCSNAME, clName, false, false ) ;
   db.removeRG( groupName);

/*   var groups = getGroupsWithNodeNum( 3 );
   if( groups.length === 0 )
   {
      return;
   }
   var group = groups[0];
   var groupName = group[0].GroupName;
   var primaryPos = group[0].PrimaryPos;println(primaryPos);
   var clName = CHANGEDPREFIX + "_14102";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName });
   insertData( cl );

   var options = { PreferedInstance: [ primaryPos, "S"] };
   var expAccessNodes = [ group[ primaryPos ].HostName + ":" + group[ primaryPos ].svcname ];
   checkAccessNodes( cl, expAccessNodes, options );
 
   primaryPos == 1 ? slavePos = 2 : slavePos = 1;
   options = { PreferedInstance: [ slavePos, "M"] };
   var expAccessNodes = [ group[ slavePos ].HostName + ":" + group[ slavePos ].svcname ];
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false ) ;*/
}

