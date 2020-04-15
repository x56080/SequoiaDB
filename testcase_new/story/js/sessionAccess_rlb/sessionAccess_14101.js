/* *****************************************************************************
@description: seqDB-14101:多组查询，设置会话访问属性，preferedinstance值为其他组未设置instanceid的节点下标
@author: 2020-4-15 zhaoxiaoni  Init
***************************************************************************** */
main();

function main()
{
   var groups = getGroupsWithNodeNum( 3 );
   if( groups.length === 0 )
   {
      return;
   }

   var nodeNum = 3;
   var instanceidList = [ 2 ];
   var groupName1 = "rg_14101";
   var group = groups[0].sort( sortBy( "NodeID" ) );
   var groupName2 = group[0].GroupName;
   var hostName = group[2].HostName;
   var nodeNames = createRGAndNodes( db, groupName1, hostName, nodeNum, instanceidList );

   var clName = CHANGEDPREFIX + "_14101";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName1, ShardingKey: { no: 1 } } );
   insertData( cl );
   cl.split( groupName1, groupName2, 50 );

   var options = { PreferedInstance: 2 };
   var expAccessNodes = [ nodeNames[0], nodeNames[1], hostName + ":" + group[2].svcname ];
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false ) ;
   db.removeRG( groupName1 );
}

