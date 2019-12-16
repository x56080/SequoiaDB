/***************************************************************************
@Description :seqDB-12014 :range分区表中插入/更新/删除包含全文索引字段的记录 
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

   var clName = COMMCLNAME + "_ES_12014";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "range", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], { a: "a1000" }, { a: "a6000" } );

   //分区键覆盖：单分区键，索引字段覆盖：非分区键
   //插入包含全文索引字段的记录
   commCreateIndex( dbcl, "fullIndex_12014", { b: "text" } );
   commCheckIndex( dbcl, "fullIndex_12014", true );
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: "a" + i, b: "b" + i };
      records.push( record );
   }
   dbcl.insert( records );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10000 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================非分区键插入成功================" );

   //更新包含全文索引字段的记录
   dbcl.update( { $set: { b: "fullindex" } }, { b: "b1" } );
   dbcl.insert( { a: "a10001", b: "b10001" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10001 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================非分区键更新成功================" );

   //删除包含全文索引字段的记录
   dbcl.remove( { b: "fullindex" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10000 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_12014" );
   commDropIndex( dbcl, "fullIndex_12014" );
   commCheckIndex( dbcl, "fullIndex_12014", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================非分区键删除成功================" );
   println( "================================full index Not on ShardingKey================================" );

   //分区键覆盖：单分区键，索引字段覆盖：分区键
   //插入包含全文索引字段的记录
   commCreateIndex( dbcl, "fullIndex_12014", { a: "text" } );
   commCheckIndex( dbcl, "fullIndex_12014", true );

   dbcl.insert( { a: "about" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10001 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================分区键插入成功================" );

   //更新包含全文索引字段的记录
   dbcl.update( { $set: { a: "fullindex" } }, { a: "a2" } );
   dbcl.insert( { a: "a10002", b: "b10002" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10002 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================分区键更新成功================" );

   //删除包含全文索引字段的记录
   dbcl.remove( { a: "a2" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10001 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_12014" );
   commDropIndex( dbcl, "fullIndex_12014" );
   commCheckIndex( dbcl, "fullIndex_12014", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================分区键删除成功================" );
   println( "================================full index on ShardingKey================================" );

   //分区键覆盖：多分区键，索引字段覆盖：非分区键
   //插入包含全文索引字段的记录
   commDropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "range", ShardingKey: { a: 1, c: 1 }, Group: groups[0][0]["GroupName"] } );
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], { a: "a1000" }, { a: "a6000" } );

   commCreateIndex( dbcl, "fullIndex_12014", { b: "text" } );
   commCheckIndex( dbcl, "fullIndex_12014", true );
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: "a" + i, b: "b" + i, c: "c" + i };
      records.push( record );
   }
   dbcl.insert( records );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10000 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================多分区键 非分区键插入成功================" );

   //更新包含全文索引字段的记录
   dbcl.update( { $set: { b: "fullindex" } }, { b: "b1" } );
   dbcl.insert( { a: "a10001", b: "b10001", c: "c10001" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10001 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================多分区键 非分区键更新成功================" );

   //删除包含全文索引字段的记录
   dbcl.remove( { b: "fullindex" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10000 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_12014" );
   commDropIndex( dbcl, "fullIndex_12014" );
   commCheckIndex( dbcl, "fullIndex_12014", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================多分区键 非分区键删除成功================" );
   println( "================================full index Not on ShardingKey================================" );

   //分区键覆盖：单分区键，索引字段覆盖：分区键
   //插入包含全文索引字段的记录
   commCreateIndex( dbcl, "fullIndex_12014", { a: "text", c: "text" } );
   commCheckIndex( dbcl, "fullIndex_12014", true );

   dbcl.insert( { a: "about", c: "content" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10001 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================多分区键 分区键插入成功================" );

   //更新包含全文索引字段的记录
   dbcl.update( { $set: { a: "fullindex" } }, { a: "a2" } );
   dbcl.insert( { a: "a10002", b: "b10002", c: "c10002" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10002 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================多分区键 分区键更新成功================" );

   //删除包含全文索引字段的记录
   dbcl.remove( { a: "a2" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10001 );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_12014" );
   commDropIndex( dbcl, "fullIndex_12014" );
   commCheckIndex( dbcl, "fullIndex_12014", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================多分区键 分区键删除成功================" );
   println( "================================full index on ShardingKey================================" );

   commDropCL( db, COMMCSNAME, clName, true, true );
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

