/************************************
 * *@Description: 节点重启后，dropIndex检查统计信息 
 * *@author:      luweikang
 * *@createdate:  2019.06.01
 * *@testlinkCase:seqDB-18385
 * **************************************/
import( "../analyze_sync/commlib.js" );
var csName = "cs18385";
var groupName = "group18385";
var svcName = "";

main();

function main ()
{
   //判断独立模式
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var clName = "cl18385";
   var indexName = "index18385";
   var hostName = getHostName();
   var srcLogPath = "";

   try
   {
      println( "---create rg and node---" );
      var rg = db.createRG( groupName );
      srcLogPath = createData( rg, hostName );

      println( "---create cs and cl, insert record and create index---" );
      var cl = db.createCS( csName ).createCL( clName, { "Group": groupName } );
      cl.createIndex( indexName, { "a": 1 } );
      for( var i = 0; i < 1000; i++ )
      {
         cl.insert( { a: i } );
      }

      println( "---analyze---" );
      db.analyze();

      var data = new Sdb( hostName, svcName );
      var cur = data.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ).find( { "Index": indexName } );
      if( !cur.next() )
      {
         throw buildException( "", "", "create index and analyze, data node shuold be had index info", "exist", "not exist" );
      }

      println( "---restart group---" );
      rg.stop();
      rg.start();

      println( "---drop index---" );
      db.getCS( csName ).getCL( clName ).dropIndex( indexName );

      println( "---check data node SYSSTAT.SYSINDEXSTAT---" );
      data = new Sdb( hostName, svcName );
      cur = data.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ).find( { "Index": indexName } );
      if( cur.next() )
      {
         throw buildException( "", "", "drop index, data node shuold be no index info", "not exist", "exist" );
      }
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/18385";
      File.mkdir( backupDir );
      File.scp( srcLogPath, backupDir + "/sdbdiag.log" );
      throw e;
   }
   finally
   {
      commDropCS( db, csName, true, "drop CS in the end." );
      db.removeRG( groupName );
   }
}

function getHostName ()
{
   var groupArray = commGetGroups( db );
   var hostName = groupArray[0][1].HostName;
   return hostName;
}

function createData ( rg, hostName )
{
   svcName = parseInt( RSRVPORTBEGIN ) + 10;
   var dataPath = RSRVNODEDIR + "data/" + svcName;
   var checkSucc = false;
   var times = 0;
   var maxRetryTimes = 10;
   do
   {
      try
      {
         rg.createNode( hostName, svcName, dataPath, { diaglevel: 5 } );
         checkSucc = true;
      }
      catch( e )
      {
         //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
         if( e == -145 || e == -290 )
         {
            svcName = svcName + 10;
            dataPath = RSRVNODEDIR + "data/" + svcName;
         }
         else
         {
            throw "create node failed!  svcName = " + svcName + " dataPath = " + dataPath + " errorCode: " + e;
         }
         times++;
      }
   }
   while( !checkSucc && times < maxRetryTimes );
   rg.start();
   srcLogPath = hostName + ":" + CMSVCNAME + "@" + dataPath + "/diaglog/sdbdiag.log";
   return srcLogPath;
}
