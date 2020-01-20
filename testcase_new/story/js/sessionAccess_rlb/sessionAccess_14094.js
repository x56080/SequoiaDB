/* *****************************************************************************
@description: seqDB-14094:设置会话访问属性，指定preferedinstance值instanceid不存在对应节点和[S/M/A]
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test()
{
   var groups = getGroupsWithNodeNum( 3 );
   if( groups.length === 0 )
   {
      return;
   }
   var group = groups[0];
   var groupName = group[0].GroupName;
   var primaryPos = group[0].PrimaryPos;
   var clName = CHANGEDPREFIX + "_14094";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName });
   insertData( cl );
   var options = { PreferedInstance: [11, 224, 38, "M"] };
   var expAccessNodes = [ group[ primaryPos ].HostName + ":" + group[ primaryPos ].svcname ];
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "m"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "S"] };
   expAccessNodes = [];
   for( var i = 1; i < group.length; i++ )
   {
      if( i !== primaryPos )
      {
         expAccessNodes.push( group[i]["HostName"] + ":" + group[i]["svcname"]);
      }
   }
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "s"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "A"] };
   expAccessNodes = getGroupNodes( groupName );
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "a"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 30, "A", "M", "S"] };
   checkAccessNodes( cl, expAccessNodes, options );

   options = { PreferedInstance: [11, 224, 38, "M", "S", "A"] };
   expAccessNodes = [ group[ primaryPos ].HostName + ":" + group[ primaryPos ].svcname ];
   checkAccessNodes( cl, expAccessNodes, options );

   commDropCL( db, COMMCSNAME, clName, false, false ) ;
}
                                                                            
