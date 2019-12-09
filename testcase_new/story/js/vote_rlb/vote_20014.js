/* *****************************************************************************
@discretion: seqDB-20014:修改组内其中一个节点的权重为100，重新选主
@author：2018-11-06 zhaoxiaoni
***************************************************************************** */
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   try
   {
      var groupName = "rg_20014";
      var nodesNum = 3;
      var isGroupCreated = createGroupAndNode( db, groupName, nodesNum );

      var csName = "cs_20014";
      var clName = "cl_20014";
      var cl = db.createCS( csName ).createCL( clName, { Group: groupName } );
      var data = [];
      for( var i = 0; i < 10000; i++ )
      {
         data.push( { a: i } );
      }
      cl.insert( data );

      var dataNodeAttr = addNode( groupName );
      var hostName = dataNodeAttr["hostName"];
      var svcName = dataNodeAttr["svcName"];
      var dbPath = dataNodeAttr["dbPath"];

      println( "wait sycn start" );
      waitSync( hostName, groupName, svcName );

      //节点创建成功启动后会有两个心跳窗口时间（一个心跳窗口时间是7s）的静默期，此时不会接受选主消息，因此这里停14s再执行选主
      sleep( 14000 );

      db.getRG( groupName ).reelect( { Seconds: 60 } );
      checkReelect( groupName, hostName, svcName );

      db.getRG( groupName ).reelect( { Seconds: 60 } );//由于内部实现影子权重的作用，再次reelect将切换主节点
   }
   catch( e )
   {
      throw new Error( e );
   }
   finally
   {
      if( isGroupCreated )
      {
         db.dropCS( csName );
         db.removeRG( groupName );
      }
   }
}

function addNode ( groupName )
{
   var dataNodeAttr = {};
   dataNodeAttr.hostName = System.getHostName();
   dataNodeAttr.config = { weight: 100, diaglevel: 5 };
   var doTimes = 0;
   var checkSucc = false;
   var maxRetryTimes = 10;
   while( !checkSucc && doTimes < maxRetryTimes )
   {
      dataNodeAttr.svcName = parseInt( RSRVPORTBEGIN ) + 10 * ( 3 + doTimes );
      dataNodeAttr.dbPath = RSRVNODEDIR + "data/" + dataNodeAttr.svcName;
      try
      {
         println( "add node: " + dataNodeAttr.hostName + ":" + dataNodeAttr.svcName + ", dbpath: " + dataNodeAttr.dbPath );
         db.getRG( groupName ).createNode( dataNodeAttr.hostName, dataNodeAttr.svcName, dataNodeAttr.dbPath, dataNodeAttr.config );
         checkSucc = true;
      }
      catch( e )
      {
         if( e == -145 || e == -290 )
         {
            doTimes++;
         }
      }
   }
   db.getRG( groupName ).start();

   return dataNodeAttr;
}

function checkReelect ( groupName, hostName, svcName )
{
   println( "start check reelect" );
   var masterNode = db.getRG( groupName ).getMaster();
   var masterNodeHostName = masterNode.getHostName();
   var masterNodeSvcName = masterNode.getServiceName();
   if( masterNodeHostName != hostName || masterNodeSvcName != svcName )
   {
      throw new Error( "Reelect failed!" );
   }
   println( "check reelect finished" );
}

function getLSN ( db )
{
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   var completeLSN = cursor.current().toObj()["CompleteLSN"];
   return completeLSN;
}

function waitSync ( hostName, groupName, svcName )
{
   var masterNode = db.getRG( groupName ).getMaster().connect();
   var newNode = new Sdb( hostName, svcName );
   var doTimes = 0;
   var timeout = 300;
   while( doTimes < timeout )
   {
      var masterNodeLSN = getLSN( masterNode );
      var newNodeLSN = getLSN( newNode );
      if( masterNodeLSN <= newNodeLSN )
      {
         sleep( 1000 );
         doTimes++;
      }
      else
      {
         break;
      }
      println( "masterNodeLSN:" + masterNodeLSN );
      println( "newNodeLSN:" + newNodeLSN );
   }
   println( "wait sync finished" );
}

function createGroupAndNode ( db, rgName, nodesNum )
{
   var rg = db.createRG( rgName );
   var isGroupCreated = true;
   var host = System.getHostName();
   var failedCount = 0;
   for( var i = 0; i < nodesNum; i++ )
   {
      var svc = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount );
      var dbPath = RSRVNODEDIR + "data/" + svc;
      var checkSucc = false;
      var times = 0;
      var maxRetryTimes = 10;
      do
      {
         try
         {
            rg.createNode( host, svc, dbPath, { diaglevel: 5 } );
            println( "create node: " + host + ":" + svc + " dbpath: " + dbPath );
            checkSucc = true;
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( e == -145 || e == -290 )
            {
               svc = svc + 10;
               dbPath = RSRVNODEDIR + "data/" + svc;
               failedCount++;
            }
            else
            {
               throw "create node failed!  port = " + svc + " dataPath = " + dbPath + " errorCode: " + e;
            }
            times++;
         }
      }
      while( !checkSucc && times < maxRetryTimes );
   }
   println( "start group" );
   rg.start();
   return isGroupCreated;
}
