/******************************************************************************
 * @Description : test delete multi config in a group 
 *                seqDB-14843:指定一个组批量删除相同级别的配置
 * @author      : Liang XueWang 
 ******************************************************************************/
import( "../configs/commlib.js" );
// 删除多个run级别参数
function deleteRun ( db, rgName, nodes )
{
   // pick two random run conf
   var runConf1 = getRandomRunConf();
   do
   {
      var runConf2 = getRandomRunConf();
   } while( runConf2.name === runConf1.name );
   println( "test run conf: " + runConf1 + " " + runConf2 );

   // before delete, get nodes conf from file and snapshot
   var snapshotInfos = [];
   var fileInfos = [];
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var info = getConfFromSnapshot( host, svc );
      snapshotInfos.push( info );
      info = getConfFromFile( host, svc );
      fileInfos.push( info );
   }

   // delete run conf
   var config = {};
   config[runConf1.name] = 1;
   config[runConf2.name] = 1;
   var option = {};
   option["GroupName"] = rgName;
   deleteConf( db, config, option );

   // check delete with snapshot and file
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var snapshotInfo = getConfFromSnapshot( host, svc );
      var expConfs = {};
      expConfs[runConf1.name] = runConf1.defVal;
      expConfs[runConf2.name] = runConf2.defVal;
      checkSnapshot( snapshotInfos[i], snapshotInfo, expConfs );
      var fileInfo = getConfFromFile( host, svc );
      checkConfFile( fileInfos[i], fileInfo );
   }

   // update run conf to origin value
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var config = {};
      config[runConf1.name] = snapshotInfos[i][runConf1.name];
      config[runConf2.name] = snapshotInfos[i][runConf2.name];
      var option = {};
      option["HostName"] = host;
      option["svcname"] = svc;
      updateConf( db, config, option );
   }

   // check update with snapshot and file again
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var snapshotInfo = getConfFromSnapshot( host, svc );
      checkSnapshot( snapshotInfos[i], snapshotInfo );
      var fileInfo = getConfFromFile( host, svc );
      checkConfFile( fileInfos[i], fileInfo );
   }
}

// 删除多个reboot级别参数
function deleteReboot ( db, rgName, nodes )
{
   // pick two random reboot conf
   var rebootConf1 = getRandomRebootConf();
   do
   {
      var rebootConf2 = getRandomRebootConf();
   } while( rebootConf2.name === rebootConf1.name );
   println( "test reboot conf: " + rebootConf1 + " " + rebootConf2 );

   // before delete, get nodes conf from file and snapshot
   var snapshotInfos = [];
   var fileInfos = [];
   var hasErr = false;
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var info = getConfFromSnapshot( host, svc );
      if( info[rebootConf1.name] !== rebootConf1.defVal ||
         info[rebootConf2.name] !== rebootConf2.defVal )
      {
         hasErr = true;
      }
      snapshotInfos.push( info );
      info = getConfFromFile( host, svc );
      fileInfos.push( info );
   }

   // delete reboot conf
   var config = {};
   config[rebootConf1.name] = 1;
   config[rebootConf2.name] = 1;
   var option = {};
   option["GroupName"] = rgName;
   if( hasErr )
   {
      deleteConf( db, config, option, -264 );
   }
   else
   {
      deleteConf( db, config, option );
   }

   // check delete with snapshot and file
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var snapshotInfo = getConfFromSnapshot( host, svc );
      checkSnapshot( snapshotInfos[i], snapshotInfo );
      var fileInfo = getConfFromFile( host, svc );
      checkConfFile( fileInfos[i], fileInfo );
   }

   // update reboot conf to origin value
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var config = {};
      config[rebootConf1.name] = snapshotInfos[i][rebootConf1.name];
      config[rebootConf2.name] = snapshotInfos[i][rebootConf2.name];
      var option = {};
      option["HostName"] = host;
      option["svcname"] = svc;
      updateConf( db, config, option );
   }

   // check update with snapshot and file again
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var snapshotInfo = getConfFromSnapshot( host, svc );
      checkSnapshot( snapshotInfos[i], snapshotInfo );
      var fileInfo = getConfFromFile( host, svc );
      var expConfs = {};
      expConfs[rebootConf1.name] = snapshotInfos[i][rebootConf1.name];
      expConfs[rebootConf2.name] = snapshotInfos[i][rebootConf2.name];
      checkConfFile( fileInfos[i], fileInfo, expConfs );
   }
}

// 删除多个forbid级别参数
function deleteForbid ( db, rgName, nodes )
{
   // before delete, get nodes conf from file and snapshot
   var snapshotInfos = [];
   var fileInfos = [];
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var info = getConfFromSnapshot( host, svc );
      snapshotInfos.push( info );
      info = getConfFromFile( host, svc );
      fileInfos.push( info );
   }

   // delete forbid conf
   var config = {};
   config['dbpath'] = 1;
   config['indexpath'] = 1;
   var option = {};
   option["GroupName"] = rgName;
   deleteConf( db, config, option, -264 );

   // check delete with snapshot and file
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var snapshotInfo = getConfFromSnapshot( host, svc );
      checkSnapshot( snapshotInfos[i], snapshotInfo );
      var fileInfo = getConfFromFile( host, svc );
      checkConfFile( fileInfos[i], fileInfo );
   }
}

function main ( db )
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }

   // create a group with 3 nodes
   var rgName = "testGroup14843";
   var nodesNum = 3;
   var logSourcePaths = [];
   try
   {
      logSourcePaths = createAndStartGroup( db, rgName, nodesNum );
      nodes = getGroupNodes( db, rgName );

      // test delete multi run conf in a group
      deleteRun( db, rgName, nodes );

      // test delete multi reboot conf in a group
      deleteReboot( db, rgName, nodes );

      // test delete multi forbid conf in a group 
      deleteForbid( db, rgName, nodes );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/14843";
      File.mkdir( backupDir );
      for( var i = 0; i < logSourcePaths.length; i++ )
      {
         File.scp( logSourcePaths[i], backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      //清理环境
      removeGroup( db, rgName );
   }
}

main( db );