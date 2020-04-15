/* *****************************************************************************
@description: seqDB-14085:设置会话属性，preferedinstance指定instanceid与其它节点下标相同的实例 
@author: 2020-4-15 zhaoxiaoni Init
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
   var groupName = "rg_14085";
   var instanceidList = [ 2 ];
   var clName = CHANGEDPREFIX + "_14085";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeNames = createRGAndNodes( db, groupName, hostName, nodeNum, instanceidList );

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 });
   insertData( cl );

   var instanceid = 2;
   var expAccessNodes = [ nodeNames[0], nodeNames[1] ];
   var options = { PreferedInstance: instanceid };
   checkAccessNodes( cl, expAccessNodes, options );
   
   commDropCL( db, COMMCSNAME, clName, false, false );
   db.removeRG( groupName );
}

