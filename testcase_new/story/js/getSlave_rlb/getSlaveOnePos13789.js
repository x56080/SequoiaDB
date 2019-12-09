/******************************************************************************
*@Description : test getSlave operation
*               seqDB-13789:指定一个位置获取备节点
*@auhor       : Liang XueWang
******************************************************************************/
var rgName = "testGetSlaveRg13789";
var logSourcePaths = [];
main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }

   try
   {
      testOneNode();
      testTwoNode();
      testTwoNodeNoMaster();
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/13789";
      File.mkdir( backupDir );
      for( var i = 0; i < logSourcePaths.length; i++ )
      {
         File.scp( logSourcePaths[i], backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      db.removeRG( rgName );
   }
}

// only one node in rg, test getSlave
function testOneNode ()
{
   var rg = db.createRG( rgName );
   logSourcePaths = createNodes( rg, 1 );

   var master = getMaster( rg );

   for( var i = 1; i <= 7; i++ )
   {
      var slave = rg.getSlave( i );
      if( slave.toString() !== master.toString() )
      {
         throw buildException( "testOneNode", null, "check getSlave", master, slave );
      }
   }
}

// two nodes in rg, test getSlave
function testTwoNode ()
{
   var rg = db.getRG( rgName );
   logSourcePaths = createNodes( rg, 1 );

   var master = getMaster( rg );

   var nodes = getGroupNodes( db, rgName );
   for( var i = 1; i <= 7; i++ )
   {
      var slave = rg.getSlave( i );
      var idx = ( i - 1 ) % 2;
      if( slave.toString() !== nodes[idx] )
      {
         throw buildException( "testTwoNode", null, "check getSlave", nodes[idx], slave );
      }
   }
}

// two nodes in rg( no master ), test getSlave
function testTwoNodeNoMaster ()
{
   var rg = db.getRG( rgName );

   var master = getMaster( rg );
   master.stop();
   var hasMaster = isMasterExist( db, rgName );
   if( hasMaster !== false )
   {
      throw buildException( "testTwoNodeNoMaster", null, "check no master", false, hasMaster );
   }

   var nodes = getGroupNodes( db, rgName );
   for( var i = 1; i <= 7; i++ )
   {
      var slave = rg.getSlave( i );
      var idx = ( i - 1 ) % 2;
      if( slave.toString() !== nodes[idx] )
      {
         throw buildException( "testTwoNodeNoMaster", null, "check getSlave", nodes[idx], slave );
      }
   }
}