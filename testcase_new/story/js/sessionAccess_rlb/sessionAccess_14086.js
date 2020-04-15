/******************************************************************************
@description: seqDB-14086:设置会话访问属性preferedinstance为多个不同值，preferedinstanceMode覆盖不同值
@author: 2020-4-15 zhaoxiaoni  Init
***************************************************************************** */
main();

function main()
{
   if( commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var nodeNum = 3;
   var groupName = "rg_14086";
   var instanceidList = [ 22, 23 ];
   var clName = CHANGEDPREFIX + "_14086";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeNames = createRGAndNodes( db, groupName, hostName, nodeNum, instanceidList );

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 });
   insertData( cl );

   //设置preferedinstancemode为random
   var expAccessNodes = [ nodeNames[0], nodeNames[1] ]; 
   var options = { PreferedInstance: instanceidList, PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   //不设置preferedinstancemode
   options = { PreferedInstance: instanceidList };
   checkAccessNodes( cl, expAccessNodes, options );
 
   //设置preferedinstancemode为ordered
   expAccessNodes.pop();
   options = { PreferedInstance: instanceidList, PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false );
   db.removeRG( groupName );
}

