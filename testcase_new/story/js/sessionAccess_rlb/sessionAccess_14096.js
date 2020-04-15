/* *****************************************************************************
@description: seqDB-14096:设置会话访问属性，指定preferedinstance包含【8/9/10】
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
   var groupName = "rg_14096";
   var instanceidList = [ 8, 9, 10 ];
   var clName = CHANGEDPREFIX + "_14093";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeNames = createRGAndNodes( db, groupName, hostName, nodeNum, instanceidList );
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 });
   insertData( cl );
  
   var options = { PreferedInstance: 8 };
   var expAccessNodes = [ nodeNames[0] ];
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: 9 };
   var expAccessNodes = [ nodeNames[1] ];
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: 10 };
   var expAccessNodes = [ nodeNames[2] ];
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false ) ;
   db.removeRG( groupName );
}
