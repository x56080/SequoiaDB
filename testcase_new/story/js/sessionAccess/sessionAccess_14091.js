/* *****************************************************************************
@description: seqDB-14091:设置会话访问属性，多值指定preferedinstance为[M/S/A]，preferedinstancemode覆盖random和ordered
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

// main( test );

function test ( testPara )
{
   var groups = getGroupsWithNodeNum( 2 );
   if( groups.length === 0 )
   {
      return;
   }
   var group = groups[0];

   var groupName = group[0].GroupName;
   var primaryPos = group[0].PrimaryPos;
   var clName = CHANGEDPREFIX + "_14091";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 } );
   insertData( cl );

   //PreferedInstance为["A","M","S"]，PreferedInstanceMode为random
   var options = { PreferedInstance: ["A", "M", "S"], PreferedInstanceMode: "random" };
   var expAccessNodes = getGroupNodes( groupName );
   checkAccessNodes( cl, expAccessNodes, options );

   //PreferedInstance为["M","S","A"]，PreferedInstanceMode为ordered
   options = { PreferedInstance: ["M", "S", "A"], PreferedInstanceMode: "ordered" };
   var expAccessNodes = [group[primaryPos].HostName + ":" + group[primaryPos].svcname];
   checkAccessNodes( cl, expAccessNodes, options );

   //PreferedInstance为["A","M","S"]，不设置PreferedInstanceMode
   options = { PreferedInstance: ["A", "M", "S"] };
   var expAccessNodes = getGroupNodes( groupName );
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, true, true );
}

