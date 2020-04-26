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

   options = { PreferedInstance: "S" };
   //校验访问备节点负载均衡
   checkBalance( cl, primaryNode, options );
  
   options = { PreferedInstance: "-S" };
   //校验访问备节点负载均衡
   checkBalance( cl, primaryNode, options );

   expAccessNodes = getGroupNodes( groupName );
   options =  { PreferedInstance: "A" };
   checkAccessNodes( cl, expAccessNodes, options );

   options =  { PreferedInstance: "-A" };
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false ) ;
}

function checkBalance( cl, primaryNode, options )
{
   var actAccessNodes = {};
   for( var i = 0; i < 1000; i++ )
   {   
      db.setSessionAttr( options );
      var cursor = cl.find().explain();
      while( cursor.next() )
      {
         var actAccessNode = cursor.current().toObj().NodeName;
         if( actAccessNode == primaryNode )
         {
            throw new Error( "actAccessNode is primary node: " + primaryNode );
         }
         actAccessNodes[ actAccessNode ] == undefined ? actAccessNodes[ actAccessNode ] = 1 :
                                          actAccessNodes[ actAccessNode ] = actAccessNodes[ actAccessNode ] + 1;        
      }

   }

   //跟开发确认并没有固定的均衡率，因此暂时认为访问节点差值大于20%为负载不均衡
   var num =  actAccessNodes[ Object.keys( actAccessNodes )[0] ] - actAccessNodes[ Object.keys( actAccessNodes )[1] ];
   if( Math.abs( num ) > 100 )
   {
      throw new Error( "num:" + num );
   }
}

