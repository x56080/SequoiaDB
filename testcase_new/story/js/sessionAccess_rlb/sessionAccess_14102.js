/* *****************************************************************************
@description: seqDB-14102:设置会话访问属性，指定preferedinstance值instanceid所在节点为主/备，同时指定S/M 
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
   var groupName = "rg_14102";
   var instanceidList = [ 30, 31 ];
   var clName = CHANGEDPREFIX + "_14102";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeNames = createRGAndNodes( db, groupName, hostName, nodeNum, instanceidList );
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 });
   insertData( cl );
  
   var expAccessNodes = [ nodeNames[0] ];
   var options = { PreferedInstance: [30, "S"] };
   checkAccessNodes( cl, expAccessNodes, options );
  
   expAccessNodes = [ nodeNames[1] ];
   options = { PreferedInstance: [31, "M"] };
   checkAccessNodes( cl, expAccessNodes, options );
 
   commDropCL( db, COMMCSNAME, clName, false, false ) ;
   db.removeRG( groupName);
}

