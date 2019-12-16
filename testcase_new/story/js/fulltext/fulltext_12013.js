/***************************************************************************
@Description :seqDB-12013 :自动切分的hash分区表中插入/更新/删除包含全文索引字段的记录 
@Modify list :
              2018-11-21  YinZhen  Create
****************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      println( "Deploy one group" );
      return;
   }
   domainName = "testDomain";
   commDropDomain( db, domainName );
   commCreateDomain( db, domainName, [groups[0][0]["GroupName"], groups[1][0]["GroupName"]] );
   var clName = COMMCLNAME + "_ES_12013";
   var csName = "cs12013";
   commDropCL( db, csName, clName, true, true );
   commDropCS( db, csName );

   var dbcs = db.createCS( csName, { Domain: domainName } );
   var dbcl = commCreateCL( db, csName, clName, { ShardingType: "hash", ShardingKey: { a: 1 }, AutoSplit: true } );

   //索引字段覆盖：非分区键
   //插入包含全文索引字段的记录
   commCreateIndex( dbcl, "fullIndex_12013", { b: "text" } );
   commCheckIndex( dbcl, "fullIndex_12013", true );
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: "a" + i, b: "b" + i };
      records.push( record );
   }
   dbcl.insert( records );
   checkFullSyncToES( csName, clName, "fullIndex_12013", 10000 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );
   println( "================非分区键插入成功================" );

   //更新包含全文索引字段的记录
   dbcl.update( { $set: { b: "fullindex" } }, { b: "b1" } );
   dbcl.insert( { a: "a10001", b: "b10001" } );
   checkFullSyncToES( csName, clName, "fullIndex_12013", 10001 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );
   println( "================非分区键更新成功================" );

   //删除包含全文索引字段的记录
   dbcl.remove( { b: "fullindex" } );
   checkFullSyncToES( csName, clName, "fullIndex_12013", 10000 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( csName, clName, "fullIndex_12013" );
   commDropIndex( dbcl, "fullIndex_12013" );
   commCheckIndex( dbcl, "fullIndex_12013", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );
   println( "================非分区键删除成功================" );
   println( "================================full index Not on ShardingKey================================" );

   //索引字段覆盖：分区键
   //插入包含全文索引字段的记录
   commCreateIndex( dbcl, "fullIndex_12013", { a: "text" } );
   commCheckIndex( dbcl, "fullIndex_12013", true );

   dbcl.insert( { a: "about" } );
   checkFullSyncToES( csName, clName, "fullIndex_12013", 10001 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );
   println( "================分区键插入成功================" );

   //更新包含全文索引字段的记录
   dbcl.update( { $set: { a: "fullindex" } }, { a: "a2" } );
   dbcl.insert( { a: "a10002", b: "b10002" } );
   checkFullSyncToES( csName, clName, "fullIndex_12013", 10002 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );
   println( "================分区键更新成功================" );

   //删除包含全文索引字段的记录
   dbcl.remove( { a: "a2" } );
   checkFullSyncToES( csName, clName, "fullIndex_12013", 10001 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( csName, clName, "fullIndex_12013" );
   commDropIndex( dbcl, "fullIndex_12013" );
   commCheckIndex( dbcl, "fullIndex_12013", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );
   println( "================分区键删除成功================" );
   println( "================================full index on ShardingKey================================" );

   commDropCS( db, csName );
   commDropDomain( db, domainName );
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

