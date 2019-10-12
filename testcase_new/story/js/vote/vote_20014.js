/* *****************************************************************************
@discretion: cl asynchronous split , check the task id
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

function main()
{  
   var hostName = System.getHostName();
   var dataNodeAttr = createNode( hostName );
   var svcName = dataNodeAttr["svcName"];
   var groupName = dataNodeAttr["groupName"]; 
   
   var installPath = commGetInstallPath();
   confPath = installPath + "/conf/local/" + svcName + "/sdb.conf";
   changeNodeConf( confPath, 100 );

   db.reloadConf({HostName: hostName, ServiceName: svcName});
   db.getRG(groupName).reelect();
   
   checkReelect( groupName, hostName, svcName );
   
   //reset nodeConf and reelect
   changeNodeConf( confPath, 1 );
   db.reloadConf({HostName: hostName, ServiceName: svcName});
   db.getRG(groupName).reelect();
   
   db.getRG(groupName).removeNode(hostName, svcName);
}

function createNode( hostName )
{
   var dataNodeAttr = {};
   var groupArr = commGetGroups( db );
   var groupName = groupArr[0][0]["GroupName"];
   var dbPath = RSRVNODEDIR + "data/" + RSRVPORTBEGIN; 
   var config = {weight: 1};
   try
   {
      db.getRG(groupName).createNode(hostName, RSRVPORTBEGIN, dbPath, config);
      db.getRG(groupName).start();
      dataNodeAttr.groupName = groupName;
      dataNodeAttr.svcName = RSRVPORTBEGIN;
   }
   catch(e)
   {
      db.getRG(groupName).removeNode(hostName, RSRVPORTBEGIN);
      throw e;
   }
   return dataNodeAttr;
}

function changeNodeConf( confPath, weight )
{
   /*var file = new File(confPath, 0777, SDB_FILE_READWRITE);
   file.seek(0, "e");
   var writeContent = "weight = " + weight;
   file.write(writeContent);*/
   var cmd = new Cmd();
   cmd.run( "sed -i 's/weight=1/weight=100/g' " + confPath );
}

function checkReelect( groupName, hostName, svcName)
{
   var masterNode = db.getRG(groupName).getMaster();
   var masterNodeHostName = masterNode.getHostName();
   var masterNodeSvcName = masterNode.getServiceName();
   if( masterNodeHostName !== hostName || masterNodeSvcName !== svcName )
   {
      throw new Error("Reelect failed!");
   }
}