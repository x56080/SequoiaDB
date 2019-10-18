/* *****************************************************************************
@discretion: seqDB-20014:修改组内其中一个节点的权重为100，重新选主
@author：2018-11-06 zhaoxiaoni
***************************************************************************** */
try
{
   main();
}
catch(e)
{
   if(e.constructor === Error)
   {
      println(e.stack);
   }
   throw e;
}

var isNodeCreated = false;
function main()
{
   if(commIsStandalone( db ))
   {
      println( "run mode is standalone" );
      return;
   }
   
   try
   {
      var dataNodeAttr = createNode();
      var hostName = dataNodeAttr["hostName"];
      var svcName = dataNodeAttr["svcName"];
      var groupName = dataNodeAttr["groupName"]; 
      var dbPath = dataNodeAttr["dbPath"];     
  
      //节点创建成功启动后会有两个心跳窗口时间（一个心跳窗口时间是7s）的静默期，此时不会接受选主消息，因此这里停14s再执行选主
      sleep(14000);
      db.getRG(groupName).reelect({Seconds: 60});
      checkReelect( groupName, hostName, svcName );
   
      db.getRG(groupName).reelect({Seconds: 60});//由于内部实现影子权重的作用，再次reelect将切换主节点
   }
   catch(e)
   { 
      if(isNodeCreated)
      {
         var srcLogPath = dbPath + "/diaglog/sdbdiag.log";
         var backupDir = "/tmp/ci/rsrvnodelog/20014";
         File.mkdir(backupDir);
         File.copy(srcLogPath, backupDir + "/sdbdiag.log");
      }
      throw e;
   }
   finally
   {
      if(isNodeCreated)
      {
         db.getRG(groupName).removeNode(hostName, svcName)
      }
   }
}

function createNode( )
{
   var dataNodeAttr = {};
   var groupArr = commGetGroups( db );
   dataNodeAttr.hostName = System.getHostName();
   dataNodeAttr.groupName = groupArr[0][0]["GroupName"];
   dataNodeAttr.svcName = RSRVPORTBEGIN;
   dataNodeAttr.dbPath = RSRVNODEDIR + "data/" + dataNodeAttr.svcName; 
   dataNodeAttr.config = {weight: 100, diaglevel: 5};
   db.getRG(dataNodeAttr.groupName).createNode(dataNodeAttr.hostName, dataNodeAttr.svcName, dataNodeAttr.dbPath, dataNodeAttr.config);
   isNodeCreated = true;
   db.getRG(dataNodeAttr.groupName).start();
   
   return dataNodeAttr;
}

function checkReelect( groupName, hostName, svcName)
{
   var masterNode = db.getRG(groupName).getMaster();
   var masterNodeHostName = masterNode.getHostName();
   var masterNodeSvcName = masterNode.getServiceName();
   if( masterNodeHostName !== hostName || masterNodeSvcName !== svcName )
   {
      throw new Error( "Reelect failed!" );
   }
}
