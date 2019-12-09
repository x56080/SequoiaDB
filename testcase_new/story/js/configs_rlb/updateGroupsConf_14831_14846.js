/******************************************************************************
 * @Description : test update run config 
 *                seqDB-14831:指定多个组批量更新不同级别的配置
                  seqDB-14846:指定多个组批量删除不同级别的配置
 * @author      : Lu weikang
 * @date        ：2018.3.30
 ******************************************************************************/
import( "../configs/commlib.js" );
main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( " run mode is standalone" );
      return;
   }

   var hostName = getLocalHostName();
   var groupName1 = "group14831_1";
   var groupName2 = "group14831_2";
   var groupName3 = "group14831_3";
   var groupArr = [groupName1, groupName2, groupName3];
   var svcName1 = Number( RSRVPORTBEGIN );
   var svcName2 = Number( RSRVPORTBEGIN ) + 10;
   var svcName3 = Number( RSRVPORTBEGIN ) + 20;
   var svcNames = [];

   try
   {
      // create rg and node
      println( "create rg and node" );
      var dbPath = commGetInstallPath() + "/database/data/";
      svcNames.push( createGroupAndNode( db, hostName, groupName1, svcName1, dbPath ) );
      svcNames.push( createGroupAndNode( db, hostName, groupName2, svcName2, dbPath ) );
      svcNames.push( createGroupAndNode( db, hostName, groupName3, svcName3, dbPath ) );

      println( "update config" );
      //test update config in more group
      var index = Math.floor( Math.random() * 3 );
      println( "index : " + index );
      updateGroupConf( db, hostName, groupArr, svcNames[index] );

      deleteGroupConf( db, hostName, groupArr, svcNames[index] );

      println( "remove rg in the end" );
   }
   finally
   {
      //remove rg in the end
      db.removeRG( groupName1 );
      db.removeRG( groupName2 );
      db.removeRG( groupName3 );
   }
}

function createGroupAndNode ( db, hostName, groupName, svcName, dbPath )
{
   var checkSucc = false;
   var times = 0;
   var maxRetryTimes = 10;
   var rg = db.createRG( groupName );
   dbPath = RSRVNODEDIR + "data/" + svcName;
   do
   {
      try
      {
         rg.createNode( hostName, svcName, dbPath );
         println( "create RG node:" + hostName + "," + svcName );
         checkSucc = true;
      }
      catch( e )
      {
         //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
         if( e == -145 || e == -290 )
         {
            svcName = parseInt( svcName ) + 10;
            dbPath = RSRVNODEDIR + "data/" + svcName;
         }
         else
         {
            throw "create node failed!  port = " + svcName + " path = " + dbPath + " errorCode: " + e;
         }
         times++;
      }
   }
   while( !checkSucc && times < maxRetryTimes );
   println( "start data group" );
   rg.start();
   return svcName;
}

function updateGroupConf ( db, hostName, groupArr, svcName )
{
   var config = {};
   var runConf = {};
   var rebootConf = {};
   var conf = null;
   var options = { HostName: hostName, GroupName: groupArr }
   var snapshotInfo = getConfFromSnapshot( hostName, svcName );
   var fileInfo = getConfFromFile( hostName, svcName );

   var runConfArr = getAllRunConf();
   var rebootConfArr = getAllRebootConf();
   var forbidConfArr = getAllForbidConf();

   for( i = 0; i < runConfArr.length; i++ )
   {
      conf = runConfArr[i];
      config[conf.name] = conf.validVal;
      runConf[conf.name] = conf.validVal;
      rebootConf[conf.name] = conf.validVal;
   }
   for( i = 0; i < rebootConfArr.length; i++ )
   {
      conf = rebootConfArr[i];
      config[conf.name] = conf.validVal;
      rebootConf[conf.name] = conf.validVal;
   }
   for( i = 0; i < forbidConfArr.length; i++ )
   {
      conf = forbidConfArr[i];
      config[conf.name] = conf.validVal;
   }
   println( "updateConf: " + JSON.stringify( config ) );
   try
   {
      db.updateConf( config, options );
      throw "UPDATE_ERR";
   }
   catch( e )
   {
      if( e !== -264 )
      {
         throw buildException( "updateGroupConf", e, "update more group and more config :" + JSON.stringify( config ) +
            " and option: " + JSON.stringify( options ), -264, e );
      }
   }

   println( "check runconf snapshot" );
   var snapshotInfo1 = getConfFromSnapshot( hostName, svcName );
   checkSnapshot( snapshotInfo, snapshotInfo1, runConf );

   //reboot
   for( var i in groupArr )
   {
      println( "reboot rg: " + groupArr[i] );
      var groupName = groupArr[i];
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
   }

   println( "check rebootconf snapshot" );
   var snapshotInfo2 = getConfFromSnapshot( hostName, svcName );
   checkSnapshot( snapshotInfo, snapshotInfo2, rebootConf );

   println( "check config file" );
   var fileInfo1 = getConfFromFile( hostName, svcName );
   checkConfFile( fileInfo, fileInfo1 );

}

function deleteGroupConf ( db, hostName, groupArr, svcName )
{
   var config = {};
   var runConf = {};
   var rebootConf = {};
   var conf = null;
   var options = { HostName: hostName, GroupName: groupArr }
   var snapshotInfo = getConfFromSnapshot( hostName, svcName );
   var fileInfo = getConfFromFile( hostName, svcName );

   var runConfArr = getAllRunConf();
   var rebootConfArr = getAllRebootConf();
   var forbidConfArr = getAllForbidConf();

   for( i = 0; i < runConfArr.length; i++ )
   {
      conf = runConfArr[i];
      config[conf.name] = conf.defVal;
      runConf[conf.name] = conf.defVal;
      rebootConf[conf.name] = conf.defVal;
   }
   for( i = 0; i < rebootConfArr.length; i++ )
   {
      conf = rebootConfArr[i];
      config[conf.name] = conf.defVal;
      rebootConf[conf.name] = conf.defVal;
   }
   for( i = 0; i < forbidConfArr.length; i++ )
   {
      conf = forbidConfArr[i];
      config[conf.name] = conf.defVal;
   }
   println( "deleteConf: " + JSON.stringify( config ) );
   try
   {
      db.deleteConf( config, options );
      throw "DELETE_ERR";
   }
   catch( e )
   {
      if( e !== -264 )
      {
         throw buildException( "deleteGroupConf", e, "delete more group and more config :" + JSON.stringify( config ) +
            " and option: " + JSON.stringify( options ), -264, e );
      }
   }

   println( "check runconf snapshot" );
   var snapshotInfo1 = getConfFromSnapshot( hostName, svcName );
   checkSnapshot( snapshotInfo, snapshotInfo1, runConf );

   //reboot
   for( var i in groupArr )
   {
      println( "reboot rg: " + groupArr[i] );
      var groupName = groupArr[i];
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
   }

   println( "check rebootconf snapshot" );
   var snapshotInfo2 = getConfFromSnapshot( hostName, svcName );
   checkSnapshot( snapshotInfo, snapshotInfo2, rebootConf );

   println( "check config file" );
   var fileInfo1 = getConfFromFile( hostName, svcName );
   checkConfFile( fileInfo, fileInfo1 );

}
