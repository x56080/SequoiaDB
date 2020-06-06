/******************************************************************************
@Description :   seqDB-22263:事务操作过程中执行reelect操作
@author :   2020-6-5    wuyan  Init
******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range"};
testConf.clName = CHANGEDPREFIX + "_transaction22263";
main( test );

function test( testPara )
{
   var groupName = testPara.srcGroupName;     
   db.transBegin();
   testPara.testCL.insert( { a: 1 } );
   
   var newdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );    
   var rg = newdb.getRG( groupName );
   var slaveNode = rg.getSlave();
   slaveHostName = slaveNode.getHostName();
   slaveServiceName = slaveNode.getServiceName();
   rg.reelect({Seconds: 300, HostName: slaveHostName, ServiceName: slaveServiceName });
   
   try
   {
      
      db.transCommit();
      throw new Error( "commit should be failed!");
   }
   catch( e )
   {
      if( e.message !== "-85" )
      {
         throw e;
      }
   }
    
    newdb.close();
    checkReelect( groupName, slaveHostName, slaveServiceName );
    var cursor = testPara.testCL.find();
    commCompareResults( cursor, [] );
}

function checkReelect( groupName, hostName, svcName )
{
   var masterNode = db.getRG(groupName).getMaster();
   var masterNodeHostName = masterNode.getHostName();
   var masterNodeSvcName = masterNode.getServiceName();
   if( masterNodeHostName !== hostName || masterNodeSvcName !== svcName )
   {
      throw new Error( "Reelect failed! master node is " + JSON.stringify(masterNode) + "\n exp masternode is " + hostName + ":" + svcName );
   }
}