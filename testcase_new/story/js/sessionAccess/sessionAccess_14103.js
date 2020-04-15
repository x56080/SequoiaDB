/* *****************************************************************************
@description: seqDB-14103: 设置会话属性指定实例为备节点对应instanceid，写入数据后再查询 
@author: 2018-1-29 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

testConf.clName = CHANGEDPREFIX + "_14103";
var groupName = commGetGroups( db )[0][0]["GroupName"] ;
testConf.clOpt = { Group: groupName };

main( test );

function test( testPara )
{
   db.setSessionAttr( { PreferedInstance: "S" } )
   insertData( testPara.testCL );

   var master = db.getRG( groupName ).getMaster();
   var expAccessNode = master.getHostName() + ":" + master.getServiceName();
   var actAccessNode = testPara.testCL.find().explain().current().toObj().NodeName;
   if( actAccessNode !== expAccessNode )
   {
      throw new Error( "The expected result is " + expAccessNode + ", but the actual result is " + actAccessNode );
   }
}
