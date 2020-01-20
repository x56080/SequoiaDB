/* *****************************************************************************
@description: seqDB-14082:设置会话访问属性，单值指定preferedinstance为M/S/A
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test( )
{
   var group = commGetGroups( db )[0];
   var groupName = group[0].GroupName;
   var clName = CHANGEDPREFIX + "_14082";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );
   insertData( cl );

   var options = { PreferedInstance: "M" };
   var primaryPos = group[0].PrimaryPos;
   var expAccessNodes = [ group[primaryPos]["HostName"] + ":" + group[primaryPos]["svcname"] ];
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: "S" };
   expAccessNodes = [];
   for( var i = 1; i < group.length; i++ )
   {
      if( i !== primaryPos )
      {
         expAccessNodes.push( group[i]["HostName"] + ":" + group[i]["svcname"]);
      }
   }
   checkAccessNodes( cl, expAccessNodes, options );

   expAccessNodes = getGroupNodes( groupName );
   options =  { PreferedInstance: "A" };
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false ) ;
}

