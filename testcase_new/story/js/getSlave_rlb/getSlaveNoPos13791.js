/******************************************************************************
 * @Description : test getSlave operation
 *                seqDB-13791:使用getSlave()获取备节点 
 * @auhor       : Liang XueWang
 ******************************************************************************/
var rgName = "testGetSlaveRg13791";
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
      testTwoNodeNoMaster();
      testThreeNode();
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/13791";
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

   var slave = rg.getSlave();
   if( slave.toString() !== master.toString() )
   {
      throw buildException( "testOneNode", null, "check getSlave", master, slave );
   }
}

// two nodes in rg( no master ), test getSlave
function testTwoNodeNoMaster ()
{
   var rg = db.getRG( rgName );
   logSourcePaths = createNodes( rg, 1 );

   var master = getMaster( rg );
   master.stop();
   var hasMaster = isMasterExist( db, rgName );
   if( hasMaster !== false )
   {
      throw buildException( "testTwoNodeNoMaster", null, "check no master", false, hasMaster );
   }
   //当组中只有两个节点时，停掉主节点，无法选主，所以此时两个节点都是候选的备节点，无法判断是否有节点处于停止状态，所以可以获取到备节点信息即可，无法判断会随机选取哪一个返回。
   var slave = rg.getSlave();
}

// three nodes in rg( master change ), getSlave with 1-7
function testThreeNode ()
{
   var rg = db.getRG( rgName );
   logSourcePaths = createNodes( rg, 1 );

   var master = getMaster( rg );
   var nodes = getGroupNodes( db, rgName );
   var masterIdx = nodes.indexOf( master.toString() );

   var totalCnt = 50;
   var cnt = [0, 0, 0];
   for( var i = 0; i < totalCnt; i++ )
   {
      var slave = rg.getSlave();
      var idx = nodes.indexOf( slave.toString() );
      cnt[idx]++;
   }
   println( "cnt: " + cnt[0] + " " + cnt[1] + " " + cnt[2] );
   if( cnt[masterIdx] !== 0 )
   {
      throw buildException( "testThreeNode", null, "check master cnt", 0, cnt[masterIdx] );
   }
}
