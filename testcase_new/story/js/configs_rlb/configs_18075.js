/******************************************************************************
 * @Description :  seqDB-18075:多次执行updateConf更新不同配置项
 * @author      : luweikang
 * @date        ：2019.04.04
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test()
{
   var nodeNum = 1;
   var groupName = "rg_18075";
   var hostName = commGetGroups ( db )[0][1].HostName;
   var nodeOption = { diaglevel: 3 };
   var nodes = commCreateRG( db, groupName, nodeNum, hostName, nodeOption );

   var configs = { "transactionon": "TRUE" };
   var options = { "HostName": hostName, "svcName": nodes[0].svcname.toString() };
   updateConf( db, configs, options, -264 );
   check( nodes, configs );

   configs[ "numpreload" ] = 10;
   updateConf( db, configs, options, -264 );
   check( nodes, configs );

   configs[ "diaglevel" ] = 5;
   updateConf( db, configs, options, -264 );
   check( nodes, configs );

   configs[ "logfilesz" ] = 10;
   updateConf( db, configs, options, -264 );
   delete configs.logfilesz;
   check( nodes, configs );

   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();

   var snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( configs, snapshotInfo );
   var fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( configs, fileInfo );
  
   db.removeRG( groupName );
}

function check( nodes, configs )
{
   var nodeName = nodes[0].hostname + ":" + nodes[0].svcname;
   var sdbSnapshotOption = new SdbSnapshotOption().cond({ NodeName: nodeName }).options({ "mode": "local", "expand": false });
   var snapshotInfo = db.snapshot( SDB_SNAP_CONFIGS, sdbSnapshotOption ).next().toObj();
   for( var key in configs )
   {
      if( configs[key].toString().toUpperCase() !== snapshotInfo[key].toString().toUpperCase() )
      {
         throw new Error( "The expected result is " + configs[key].toString().toUpperCase() + ", but the actual resutl is " +
                           snapshotInfo[key].toString().toUpperCase() );
      }
   } 
}
