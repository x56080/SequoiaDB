/* *****************************************************************************
@description: seqDB-14082:设置会话访问属性，单值指定preferedinstance为M/S/A/-M/-S/-A
@author: 2018-1-24 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

var group = commGetGroups( db )[0];
var groupName = group[0].GroupName;
var primaryPos = group[0].PrimaryPos;
var primaryNode = group[primaryPos]["HostName"] + ":" + group[primaryPos]["svcname"];
testConf.clName = CHANGEDPREFIX + "_14082";
testConf.clOpt = { Group: groupName };

main( test );

function test( testPara )
{
   insertData( testPara.testCL );

   var options = { PreferedInstance: "M" };
   var expAccessNodes = [ primaryNode ];
   checkAccessNodes( testPara.testCL, expAccessNodes, options );

   options = { PreferedInstance: "-M" };
   checkAccessNodes( testPara.testCL, expAccessNodes, options );

   expAccessNodes = [];
   for( var i = 1; i < group.length; i++ )
   {
      if( i !== primaryPos )
      {
         expAccessNodes.push( group[i]["HostName"] + ":" + group[i]["svcname"]);
      }
   }
   options = { PreferedInstance: "S" };  
   checkAccessNodes( testPara.testCL, expAccessNodes, options );   

   options = { PreferedInstance: "-S" };
   checkAccessNodes( testPara.testCL, expAccessNodes, options );
   expAccessNodes = getGroupNodes( groupName );
   options = { PreferedInstance: "A" };
   checkAccessNodes( testPara.testCL, expAccessNodes, options );

   options = { PreferedInstance: "-A" };
   checkAccessNodes( testPara.testCL, expAccessNodes, options );
}


