/* *****************************************************************************
@description:  seqDB-14089: 设置会话访问属性，多值指定preferedinstance值instanceid部分存在，preferedinstanceMode覆盖不同值 
@author: 2020-4-15 Zhao xiaoni  Init
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
   var groupName = "rg_14089";
   var instanceidList = [ 29 ];
   var clName = CHANGEDPREFIX + "_14089";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeNames = createRGAndNodes( db, groupName, hostName, nodeNum, instanceidList );

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 });
   insertData( cl );

   //设置preferedinstancemode为random
   var instanceid = [ 29, 255 ];
   var expAccessNodes = [ nodeNames[0] ];
   var options = { PreferedInstance: instanceid, PreferedInstanceMode: "random" };
   checkAccessNodes( cl, expAccessNodes, options );

   //不设置preferedinstancemode
   options = { PreferedInstance: instanceid };
   checkAccessNodes( cl, expAccessNodes, options );

   //设置preferedinstancemode为ordered
   options = { PreferedInstance: instanceid, PreferedInstanceMode: "ordered" };
   checkAccessNodes( cl, expAccessNodes, options );
   
   commDropCL( db, COMMCSNAME, clName, false, false ) ;
   db.removeRG( groupName );
}

