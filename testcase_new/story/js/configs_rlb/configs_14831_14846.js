/******************************************************************************
 * @Description : test update run config 
 *                seqDB-14831:指定多个组批量更新不同级别的配置
                  seqDB-14846:指定多个组批量删除不同级别的配置
 * @author      : Lu weikang
 * @date        ：2018.3.30
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test()
{
   var nodeNum = 1;
   var groupNames = [ "rg_14831_14846_1", "rg_14831_14846_2", "rg_14831_14846_3" ];
   var hostName = commGetGroups ( db )[0][1].HostName;
   var nodes = commCreateRG( db, groupNames[0], nodeNum, hostName );
   nodes = nodes.concat( commCreateRG( db, groupNames[1], nodeNum, hostName ) );
   nodes = nodes.concat( commCreateRG( db, groupNames[2], nodeNum, hostName ) );

   var index = Math.floor( Math.random() * 3 );
   //指定多个组批量更新不同级别的配置 
   var configs = getConfigs( "validVal" )[ "allConfigs" ];
   var options = { GroupName: groupNames };
   updateConf ( db, configs, options, -264 );

   var runConfigs = getConfigs( "validVal" )[ "runConfigs" ];
   var rebootConfigs = getConfigs( "validVal" )[ "rebootConfigs" ];
   runRebootConfigs = JSON.parse( ( JSON.stringify( runConfigs )+JSON.stringify( rebootConfigs ) ).replace( /}{/, ",") );

   for( var i = 0; i < nodes.length; i++ )
   {
      var snapshotInfo = getConfFromSnapshot( db, nodes[i].hostname, nodes[i].svcname );
      checkResult( runConfigs, snapshotInfo );
      var fileInfo = getConfFromFile( nodes[i].hostname, nodes[i].svcname );
      checkResult( runRebootConfigs, fileInfo );
   }

   for( var i = 0; i < groupNames.length; i++ )
   {
      db.getRG( groupNames[i] ).stop();
      db.getRG( groupNames[i] ).start();
   }

   for( var i = 0; i < nodes.length; i++ )
   {
      snapshotInfo = getConfFromSnapshot( db, nodes[i].hostname, nodes[i].svcname );
      checkResult( runRebootConfigs, snapshotInfo );
      var fileInfo = getConfFromFile( nodes[i].hostname, nodes[i].svcname );
      checkResult( runRebootConfigs, fileInfo );
   }
   
   //指定多个组批量删除不同级别的配置
   var configs = getConfigs( "defaultVal" )[ "allConfigs" ];
   var options = { GroupName: groupNames };
   deleteConf( db, configs, options, -264 );

   var runConfigs = getConfigs( "defaultVal" )[ "runConfigs" ];
   var rebootConfigs = getConfigs( "defaultVal" )[ "rebootConfigs" ];
   var runRebootConfigs = JSON.parse( ( JSON.stringify( runConfigs )+JSON.stringify( rebootConfigs ) ).replace( /}{/, ",") );

   for( var i = 0; i < nodes.length; i++ )
   {
      var snapshotInfo = getConfFromSnapshot( db, nodes[i].hostname, nodes[i].svcname );
      checkResult( runConfigs, snapshotInfo );
      var fileInfo = getConfFromFile( nodes[i].hostname, nodes[i].svcname );
      checkResult( runRebootConfigs, fileInfo, true );
   }

   for( var i = 0; i < groupNames.length; i++ )
   {
      db.getRG( groupNames[i] ).stop();
      db.getRG( groupNames[i] ).start();
   }
   
   for( var i = 0; i < nodes.length; i++ )
   {
      snapshotInfo = getConfFromSnapshot( db, nodes[i].hostname, nodes[i].svcname );
      checkResult( runRebootConfigs, snapshotInfo );
      var fileInfo = getConfFromFile( nodes[i].hostname, nodes[i].svcname );
      checkResult( runRebootConfigs, fileInfo, true );
   }

   db.removeRG( groupNames[0] );
   db.removeRG( groupNames[1] );
   db.removeRG( groupNames[2] );
}
