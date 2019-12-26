/************************************
*@Description: 异常启动DB备节点不影响全文索引功能
*@author:      liuxiaoxuan
*@createdate:  2019.08.21
*@testlinkCase: seqDB-14408
**************************************/

function main ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_ES_14408";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   // 创建全文索引，插入数据
   var textIndexName = "textIndex_14408";
   dbcl.createIndex( textIndexName, { "a": "text" } );
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_14408 " + i, b: i } );
   }
   dbcl.insert( objs );

   // 异常重启数据备节点
   var groups = commGetCLGroups( db, COMMCSNAME + "." + clName );
   var preMaster = db.getRG( groups[0] ).getMaster();
   var preMasterNodeName = preMaster.getHostName() + ":" + preMaster.getServiceName();
   var preSlave = db.getRG( groups[0] ).getSlave();
   var preSlaveNodeName = preSlave.getHostName() + ":" + preSlave.getServiceName();

   var remote = new Remote( preSlave.getHostName(), CMSVCNAME );
   var cmd = remote.getCmd();
   cmd.run( "ps -ef | grep sequoiadb | grep -v grep | grep " + preSlave.getServiceName() + " | awk '{print $2}' | xargs kill -9" );

   // 等待2min，检查数据组所有节点LSN是否一致
   checkGroupBusiness( 120, COMMCSNAME, clName );

   // 执行增删改
   dbcl.insert( [{ a: 'test_14408 20001', b: 20001 }, { a: 'test_14408 20002', b: 20002 }, { a: 'test_14408 20003', b: 20003 }] );
   dbcl.update( { $set: { a: "test_14408 update" } }, { a: "test_14408 10001" } );
   dbcl.remove( { a: "test_14408 10002" } );

   // 创建全文索引，检查数据同步   
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, dbcl.count() );
   checkConsistency( COMMCSNAME, clName );

   // 走主节点执行全文检索
   var dbMaster = new Sequoiadb( preMasterNodeName );
   var masterCL = dbMaster.getCS( COMMCSNAME ).getCL( clName );
   var findConf = { "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14408" } } } } }] };
   var actResult = dbOpr.findFromCL( masterCL, findConf, { 'a': '' }, { a: 1 } );
   var expResult = dbOpr.findFromCL( masterCL, { "b": { "$lt": 10000 } }, { 'a': '' }, { a: 1 } );
   checkResult( expResult, actResult );
   println( "---check result success from master---" );
   dbMaster.close();

   // 走原备节点执行全文检索
   var dbSlave = new Sequoiadb( preSlaveNodeName );
   var slaveCL = dbSlave.getCS( COMMCSNAME ).getCL( clName );
   var actResult = dbOpr.findFromCL( slaveCL, findConf, { 'a': '' }, { a: 1 } );
   var expResult = dbOpr.findFromCL( slaveCL, { "b": { "$lt": 10000 } }, { 'a': '' }, { a: 1 } );
   checkResult( expResult, actResult );
   println( "---check result success from Slave---" );
   dbSlave.close();

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   commDropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
;
