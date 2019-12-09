/******************************************************************************
 * @Description : perform the update configuration multiple timesm specifying a different configuration each time
                  seqDB-18075:多次执行updateConfi更新不同配置项
 * @author      : luweikang
 * @date        ：2019.04.04
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
   var groupName = "group18075";
   var svcName = Number( RSRVPORTBEGIN );
   var expConfig = {};

   try
   {
      db.removeRG( groupName );
   }
   catch( e )
   {
      if( e != -154 )
      {
         throw e;
      }
   }

   try
   {
      // create rg and node
      var dbPath = RSRVNODEDIR + "data/" + svcName;
      svcName = createGroupAndNode( db, hostName, groupName, svcName, dbPath );

      //test update config
      var config = { "transactionon": true };
      expConfig["transactionon"] = true;
      testUpdateConf( db, hostName, groupName, svcName, config, expConfig );

      var config1 = { "numpreload": 10 };
      expConfig["numpreload"] = 10;
      testUpdateConf( db, hostName, groupName, svcName, config1, expConfig );

      var config2 = { "diaglevel": 5 };
      expConfig["diaglevel"] = 5;
      testUpdateConf( db, hostName, groupName, svcName, config2, expConfig );

      var config3 = { "logfilesz": 10 };
      testUpdateConf( db, hostName, groupName, svcName, config3, expConfig );

      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();

      var nodeName = hostName + ":" + svcName;
      var runOptions = new SdbSnapshotOption().cond( { NodeName: nodeName } ).options( { "mode": "run", "expand": false } )
      var afterSnapshot = getConfSnapshot( db, runOptions );

      checkSnapshotInLast( afterSnapshot, expConfig );
   }
   finally
   {
      db.removeRG( groupName );
   }
}

function createGroupAndNode ( db, hostName, groupName, svcName, dbPath )
{
   var checkSucc = false;
   var times = 0;
   var maxRetryTimes = 10;
   var rg = db.createRG( groupName );
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

function testUpdateConf ( db, hostName, groupName, svcName, config, expConfig )
{
   var nodeName = hostName + ":" + svcName;
   var localOptions = new SdbSnapshotOption().cond( { NodeName: nodeName } ).options( { "mode": "local", "expand": false } )

   var options = { HostName: hostName, GroupName: groupName }
   try
   {
      db.updateConf( config, options );
   }
   catch( e )
   {
      if( e !== -264 )
      {
         throw buildException( "updateGroupConf", e, "update config :" + JSON.stringify( config ) +
            " and option: " + JSON.stringify( options ), -264, e );
      }
   }

   var snapshotLocalInfo = getConfSnapshot( db, localOptions );
   checkSnapshotInLast( snapshotLocalInfo, expConfig );

}

function getConfSnapshot ( db, options )
{
   try
   {
      var cursor = db.snapshot( SDB_SNAP_CONFIGS, options );
      var obj = cursor.next().toObj();
   }
   catch( e )
   {
      throw buildException( "getConfFromSnapshot", e, "snapshot conf of node: " +
         options, 0, e );
   }
   if( cursor.next() !== undefined )
   {
      throw buildException( "getConfFromSnapshot", null, "snapshot conf of node " +
         options, 1, 2 );
   }
   return obj;
}

function checkSnapshotInLast ( actSnapshot, expConfig )
{
   for( var key in expConfig )
   {
      if( actSnapshot[key].toString().toUpperCase() != expConfig[key].toString().toUpperCase() )
      {
         throw buildException( "checkSnapshotInLast", null, "check local snapshot" +
            options, key + ":" + actSnapshot[key], key + ":" + expConfig[key] );
      }
   }
}
