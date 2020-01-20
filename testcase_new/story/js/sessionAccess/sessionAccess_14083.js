/* *****************************************************************************
@description: seqDB-14083:设置会话属性，选取实例instanceid值不存在 
@author: 2018-1-22 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test()
{
   var group = commGetGroups( db )[0];
   var groupName = group[0]["GroupName"] ;
   var clName = CHANGEDPREFIX + "_14083";
   commDropCL( db, COMMCSNAME, clName ) ;
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var instanceid = 40;
   var options = { PreferedInstance: instanceid };
   var nodes = group.slice(1).sort( sortBy( "NodeID" ) );
   var index = ( instanceid - 1 )%( nodes.length );
   var expAccessNodes = [ nodes[ index ].HostName + ":" + nodes[ index ].svcname ];
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [39, 12] };
   expAccessNodes = getGroupNodes( groupName );
   checkAccessNodes( cl, expAccessNodes, options );
 
   commDropCL( db, COMMCSNAME, clName, false, false );
}
