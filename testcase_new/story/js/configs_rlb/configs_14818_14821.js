/******************************************************************************
 * @Description : test update run config 
 *                seqDB-14818:更新配置文件中run级别配置
 *                seqDB-14821:更新run级别配置，且配置参数不存在conf文件中
 * @author      : Liang XueWang 
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test()
{
   var nodeNum = 1;
   var groupName = "rg_14818_14821";
   var hostName = commGetGroups ( db )[0][1].HostName;
   var nodeOption = { diaglevel: 3 };
   var nodes = commCreateRG( db, groupName, nodeNum, hostName, nodeOption );

   //当前值为默认值，修改参数值为默认值
   var config = getRandomRunConfig( "defaultVal" );
   var options = { "HostName": nodes[0].hostname, "ServiceName": nodes[0].svcname.toString() };
   updateConf( db, config, options );

   var snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( config, snapshotInfo );
   var fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( config, fileInfo );

   //删除配置参数值，将配置恢复为默认值
   deleteConf( db, config, options );

   snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( config, snapshotInfo );
   fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( config, fileInfo, true );

   //当前值为默认值，修改参数值为其他值
   config = getRandomRunConfig( "validVal" );
   options = { "HostName": nodes[0].hostname, "ServiceName": nodes[0].svcname.toString() };
   updateConf( db, config, options );

   snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( config, snapshotInfo );
   fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( config, fileInfo );

   //删除配置参数值，将配置恢复为默认值
   deleteConf( db, config, options );

   config = getRandomRunConfig( "defaultVal" );
   snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( config, snapshotInfo );
   fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( config, fileInfo, true );

   //当前值为其他值，修改参数值为无效值
   config = getRandomRunConfig( "invalidVal" );
   options = { "HostName": nodes[0].hostname, "ServiceName": nodes[0].svcname.toString() };
   updateConf( db, config, options, -264 );

   config = getRandomRunConfig( "defaultVal" );
   snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( config, snapshotInfo );
   fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( config, fileInfo, true );

   db.removeRG( groupName );
}
