/* *****************************************************************************
@description: seqDB-22075 : 设置preferedPeriod的值，执行插入操作后，检查访问计划中选取节点的情况 
@author: 2020-4-9 zhaoxiaoni  Init
***************************************************************************** */
testConf.skipStandAlone = true;

var group = commGetGroups( db )[0];
var primaryPos = group[0].PrimaryPos;
var groupName = group[0]["GroupName"];
testConf.clName = CHANGEDPREFIX + "_22075";
testConf.clOpt = { Group: groupName, ReplSize: 0 };
var primaryNode = group[primaryPos].HostName + ":" + group[primaryPos].svcname

main( test );

function test ( testPara )
{
   //preferedPeriod为0
   db.setSessionAttr( { PreferedInstance: "s", PreferedPeriod: 0 } );
   insertData( testPara.testCL );

   expAccessNodes = [];
   for( var i = 1; i < group.length; i++ )
   {
      if( i !== primaryPos )
      {
         expAccessNodes.push( group[i]["HostName"] + ":" + group[i]["svcname"] );
      }
   }
   actAccessNode = testPara.testCL.find().explain().current().toObj().NodeName;
   if( expAccessNodes.indexOf( actAccessNode ) === -1 )
   {
      throw new Error( "The expAccessNodes do not include the node: " + actAccessNode );
   }

   //preferedPeriod为2
   db.setSessionAttr( { PreferedPeriod: 2 } )
   insertData( testPara.testCL );

   actAccessNode = testPara.testCL.find().explain().current().toObj().NodeName;
   if( actAccessNode !== primaryNode )
   {
      throw new Error( "The expected result is " + expAccessNode + ", but the actual result is " + primaryNode );
   }

   //2s后检查会话访问节点为备节点
   sleep( 2000 );

   actAccessNode = testPara.testCL.find().explain().current().toObj().NodeName;
   if( expAccessNodes.indexOf( actAccessNode ) === -1 )
   {
      throw new Error( "The expAccessNodes do not include the node: " + actAccessNode );
   }

   //preferedPeriod为-1
   db.setSessionAttr( { PreferedPeriod: -1 } )
   insertData( testPara.testCL );

   actAccessNode = testPara.testCL.find().explain().current().toObj().NodeName;
   if( actAccessNode !== primaryNode )
   {
      throw new Error( "The expected result is " + expAccessNode + ", but the actual result is " + primaryNode );
   }

   //2s后检查会话访问节点还为主节点
   sleep( 2000 );

   actAccessNode = testPara.testCL.find().explain().current().toObj().NodeName;
   if( actAccessNode !== primaryNode )
   {
      throw new Error( "The expected result is " + expAccessNode + ", but the actual result is " + primaryNode );
   }
}
