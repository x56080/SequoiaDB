/******************************************************************************
 * @Description : test update multi config in a group 
 *                seqDB-14828:指定一个组批量更新相同级别的配置
 * @author      : Liang XueWang 
 ******************************************************************************/
import( "../configs/commlib.js" );
// 更新多个run级别参数
function updateRun ( db, rgName, nodes )
{
   // pick two random run conf
   var runConf1 = getRandomRunConf();
   do
   {
      var runConf2 = getRandomRunConf();
   } while( runConf2.name === runConf1.name );
   println( "test run conf: " + runConf1 + " " + runConf2 );

   // before update, get nodes conf from file and snapshot
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

   // update run conf to valid value
   var config = {};
   config[runConf1.name] = runConf1.validVal;
   config[runConf2.name] = runConf2.validVal;
   var option = {};
   option["GroupName"] = rgName;
   updateConf( db, config, option );

   // check update with snapshot and file
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var snapshotInfo = getConfFromSnapshot( host, svc );
      var expConfs = {};
      expConfs[runConf1.name] = runConf1.validVal;
      expConfs[runConf2.name] = runConf2.validVal;
      checkSnapshot( snapshotInfos[i], snapshotInfo, expConfs );
      var fileInfo = getConfFromFile( host, svc );
      checkConfFile( fileInfos[i], fileInfo, expConfs );
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

// 更新多个reboot级别参数
function updateReboot ( db, rgName, nodes )
{
   // pick two random reboot conf
   var rebootConf1 = getRandomRebootConf();
   do
   {
      var rebootConf2 = getRandomRebootConf();
   } while( rebootConf2.name === rebootConf1.name );
   println( "test reboot conf: " + rebootConf1 + " " + rebootConf2 );

   // before update, get nodes conf from file and snapshot
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

   // update reboot conf to valid value
   var config = {};
   config[rebootConf1.name] = rebootConf1.validVal;
   config[rebootConf2.name] = rebootConf2.validVal;
   var option = {};
   option["GroupName"] = rgName;
   updateConf( db, config, option, -264 );

   // check update with snapshot and file
   for( var i = 0; i < nodes.length; i++ )
   {
      var host = nodes[i].split( ":" )[0];
      var svc = nodes[i].split( ":" )[1];
      var snapshotInfo = getConfFromSnapshot( host, svc );
      checkSnapshot( snapshotInfos[i], snapshotInfo );
      var expConfs = {};
      expConfs[rebootConf1.name] = rebootConf1.validVal;
      expConfs[rebootConf2.name] = rebootConf2.validVal;
      var fileInfo = getConfFromFile( host, svc );
      checkConfFile( fileInfos[i], fileInfo, expConfs );
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

// 更新多个forbid级别参数
function updateForbid ( db, rgName, nodes )
{
   // pick two random forbid conf
   var forbidConf1 = getRandomForbidConf();
   do
   {
      var forbidConf2 = getRandomForbidConf();
   } while( forbidConf2.name === forbidConf1.name );
   println( "test forbid conf: " + forbidConf1 + " " + forbidConf2 );

   // before update, get nodes conf from file and snapshot
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

   // update forbid conf to valid value
   var config = {};
   config[forbidConf1.name] = forbidConf1.validVal;
   config[forbidConf2.name] = forbidConf2.validVal;
   var option = {};
   option["GroupName"] = rgName;
   updateConf( db, config, option, -264 );

   // check update with snapshot and file
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

   // create a group with 3 nodes and start
   var rgName = "testGroup14828";
   var nodesNum = 3;
   var logSourcePaths = [];
   try
   {
      logSourcePaths = createAndStartGroup( db, rgName, nodesNum );
      var nodes = getGroupNodes( db, rgName );

      // test update multi run conf in a group
      updateRun( db, rgName, nodes );

      // test update multi reboot conf in a group
      updateReboot( db, rgName, nodes );

      // test update multi forbid conf in a group 
      updateForbid( db, rgName, nodes );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/14828";
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