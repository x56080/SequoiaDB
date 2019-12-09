/******************************************************************************
 * @Description : test update run config 
 *                seqDB-14818:更新配置文件中run级别配置
 *                seqDB-14821:更新run级别配置，且配置参数不存在conf文件中
 * @author      : Liang XueWang 
 ******************************************************************************/
import( "../configs/commlib.js" );
// 参数为默认值，修改为其他有效值
function updateDefault ( db, host, svc )
{
   // pick a random run conf
   var runConf = getRandomRunConf();
   println( "test run conf: " + runConf );
   var confName = runConf.name;
   var defVal = runConf.defVal;
   var snapshotInfo = getConfFromSnapshot( host, svc );
   var snapshotVal = snapshotInfo[confName];
   if( snapshotVal !== defVal )
   {
      throw buildException( "updateDefault", null, "check conf " + confName,
         defVal, snapshotVal );
   }


   // before update, get conf file info
   var fileInfo = getConfFromFile( host, svc );

   // update default value to valid value
   var validVal = runConf.validVal;
   var config = {};
   config[confName] = validVal;
   var option = {};
   option["HostName"] = host;
   option["svcname"] = svc;
   updateConf( db, config, option );

   // check update with snapshot and file
   var expConfs = {};
   expConfs[confName] = validVal;
   var snapshotInfo1 = getConfFromSnapshot( host, svc );
   checkSnapshot( snapshotInfo, snapshotInfo1, expConfs );
   var fileInfo1 = getConfFromFile( host, svc );
   checkConfFile( fileInfo, fileInfo1, expConfs );

   // update valid value to default value in the end
   config[confName] = defVal;
   updateConf( db, config, option );

   // check update with snapshot and file again
   expConfs[confName] = defVal;
   var snapshotInfo2 = getConfFromSnapshot( host, svc );
   checkSnapshot( snapshotInfo1, snapshotInfo2, expConfs );
   var fileInfo2 = getConfFromFile( host, svc );
   checkConfFile( fileInfo1, fileInfo2, expConfs );
}

// 修改参数值为当前值
function updateCurrent ( db, host, svc )
{
   // pick a random run conf
   var runConf = getRandomRunConf();
   println( "test run conf: " + runConf );

   var snapshotInfo = getConfFromSnapshot( host, svc );
   var confName = runConf.name;
   var curVal = snapshotInfo[confName];

   // before update, get conf from file
   var fileInfo = getConfFromFile( host, svc );

   // update conf to current value
   var config = {};
   config[confName] = curVal;
   var option = {};
   option["HostName"] = host;
   option["svcname"] = svc;
   updateConf( db, config, option );

   // check update
   var expConfs = {};
   expConfs[confName] = curVal;
   var snapshotInfo1 = getConfFromSnapshot( host, svc );
   checkSnapshot( snapshotInfo, snapshotInfo1, expConfs );
   var fileInfo1 = getConfFromFile( host, svc );
   checkConfFile( fileInfo, fileInfo1, expConfs );
}

// 修改参数值为无效值
function updateInvalid ( db, host, svc )
{
   // pick a random run conf
   var runConf = getRandomRunConf();
   println( "test run conf: " + runConf );
   var confName = runConf.name;
   var invalidVal = runConf.invalidVal;

   // before update, get conf from snapshot and file
   var snapshotInfo = getConfFromSnapshot( host, svc );
   var fileInfo = getConfFromFile( host, svc );

   // update conf to invalid value
   var config = {};
   config[confName] = invalidVal;
   var option = {};
   option["HostName"] = host;
   option["svcname"] = svc;
   var errno = commIsStandalone( db ) ? -6 : -264;
   updateConf( db, config, option, errno );

   // check update
   var snapshotInfo1 = getConfFromSnapshot( host, svc );
   checkSnapshot( snapshotInfo, snapshotInfo1 );
   var fileInfo1 = getConfFromFile( host, svc );
   checkConfFile( fileInfo, fileInfo1 );
}

function main ( db )
{
   var hostName = getLocalHostName();

   if( commIsStandalone( db ) )
   {
      var host = ( COORDHOSTNAME === "localhost" ) ? hostName : COORDHOSTNAME;
      updateDefault( db, host, COORDSVCNAME );
      updateCurrent( db, host, COORDSVCNAME );
      updateInvalid( db, host, COORDSVCNAME );
      return;
   }

   // create data node in a group
   var groups = getDataGroups( db );
   var rgName = groups[0];
   var rg = db.getRG( rgName );
   var svcName = RSRVPORTBEGIN;
   var dbPath = commGetInstallPath() + "/database/data/" + svcName;
   var svcNameAndsrcLogPath = null;
   var srcLogPath = "";
   try
   {
      svcNameAndsrcLogPath = createAndStartNode( rg, hostName, svcName, dbPath );
      svcName = svcNameAndsrcLogPath["svcName"].toString();
      srcLogPath = svcNameAndsrcLogPath["srcLogPath"];

      // test update run conf from default value to non-default value
      updateDefault( db, hostName, svcName );

      // test update run conf to current value
      updateCurrent( db, hostName, svcName );

      // test update run conf to invalid value
      updateInvalid( db, hostName, svcName );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/14818";
      File.mkdir( backupDir );
      File.scp( srcLogPath, backupDir + "/sdbdiag.log" );
      throw e;
   }
   finally
   {
      // remove node in the end
      removeNode( rg, hostName, svcName );
   }
}

//main( db ) ;