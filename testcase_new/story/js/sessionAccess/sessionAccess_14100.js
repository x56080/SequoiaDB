/* *****************************************************************************
@description: seqDB-14100:多组查询，设置会话访问属性，z指定preferedinstance为【M/S/A】
@author: 2018-1-29 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

// main( test );

function test ()
{
   var groups = getGroupsWithNodeNum( 3 );
   if( groups.length === 0 )
   {
      return;
   }
   var group1 = groups[0];
   var group2 = groups[1];
   var groupName1 = group1[0].GroupName;
   var groupName2 = group2[0].GroupName;
   var primaryPos1 = group1[0].PrimaryPos;
   var primaryPos2 = group2[0].PrimaryPos;
   var clName = CHANGEDPREFIX + "_14100";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { no: 1 }, Group: groupName1, ReplSize: 0 } );
   insertData( cl );
   cl.split( groupName1, groupName2, 50 );

   var options = { PreferedInstance: ["M", "S", "A"] };
   var expAccessNodes = [group1[primaryPos1].HostName + ":" + group1[primaryPos1].svcname,
   group2[primaryPos2].HostName + ":" + group2[primaryPos2].svcname];
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false );
}

