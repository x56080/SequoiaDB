/* *****************************************************************************
@description: seqDB-22075 : 设置preferedPeriod的值，执行插入操作后，检查访问计划中选取节点的情况 
@author: 2020-4-9 zhaoxiaoni  Init
***************************************************************************** */
test();
function test()
{
   if( commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }   

   var clName = CHANGEDPREFIX + "_22075";
   var group = commGetGroups( db )[0];
   var primaryPos = group[0].PrimaryPos;
   var primaryNode = group[ primaryPos ].HostName + ":" + group[ primaryPos ].svcname
   var groupName = group[0]["GroupName"] ;

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, { Group: groupName }, true, true );

   //preferedPeriod为0
   db.setSessionAttr( { PreferedInstance: "S", PreferedPeriod: 0 } );
   insertData( cl );

   var expAccessNodes = [];
   for( var i = 1; i < group.length; i++ )
   {
      if( i !== primaryPos )
      {
         expAccessNodes.push( group[i]["HostName"] + ":" + group[i]["svcname"]);
      }
   }
   actAccessNode = cl.find().explain().current().toObj().NodeName;
   if( expAccessNodes.indexOf( actAccessNode ) === -1)
   {
      throw new Error( "The expAccessNodes do not include the node: " + actAccessNode );
   }

   //preferedPeriod为2
   db.setSessionAttr( { PreferedPeriod: 2 } )
   insertData( cl );

   actAccessNode = cl.find().explain().current().toObj().NodeName;
   if( actAccessNode !== primaryNode )
   {
      throw new Error( "The expected result is " + expAccessNode + ", but the actual result is " + primaryNode );
   }

   //2s后检查会话访问节点为备节点
   sleep( 2000 );

   actAccessNode = cl.find().explain().current().toObj().NodeName;
   if( expAccessNodes.indexOf( actAccessNode ) === -1)
   {
      throw new Error( "The expAccessNodes do not include the node: " + actAccessNode );
   }

   //preferedPeriod为-1
   db.setSessionAttr( { PreferedPeriod: -1 } )
   insertData( cl );

   actAccessNode = cl.find().explain().current().toObj().NodeName;
   if( actAccessNode !== primaryNode )
   {
      throw new Error( "The expected result is " + expAccessNode + ", but the actual result is " + primaryNode );
   }

   //2s后检查会话访问节点还为主节点
   sleep( 2000 );

   actAccessNode = cl.find().explain().current().toObj().NodeName;
   if( actAccessNode !== primaryNode )
   {  
      throw new Error( "The expected result is " + expAccessNode + ", but the actual result is " + primaryNode );
   }

   commDropCL( db, COMMCSNAME, clName, false, false );
}
