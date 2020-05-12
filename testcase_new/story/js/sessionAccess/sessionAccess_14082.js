/* *****************************************************************************
@description: seqDB-14082:设置会话访问属性，单值指定preferedinstance为M/S/A/-M/-S/-A
@author: 2020-4-9 zhaoxiaoni  Init
***************************************************************************** */
main();

function main()
{
   if( commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }   
   
   var group = commGetGroups( db )[0];
   var groupName = group[0].GroupName;
   var primaryPos = group[0].PrimaryPos;
   var primaryNode = group[primaryPos]["HostName"] + ":" + group[primaryPos]["svcname"];
   var clName = CHANGEDPREFIX + "_14082";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var options = { PreferedInstance: "M" };
   var expAccessNodes = [ primaryNode ];
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: "-M" };
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes = [];
   for( var i = 1; i < group.length; i++ )
   {
      if( i !== primaryPos )
      {
         expAccessNodes.push( group[i]["HostName"] + ":" + group[i]["svcname"]);
      }
   }
   options = { PreferedInstance: "S" };
   checkAccessNodes( cl, expAccessNodes, options ); 
 
   options = { PreferedInstance: "-S" };
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes = getGroupNodes( groupName );
   options = { PreferedInstance: "A" };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: "-A" };
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false ) ;
}

