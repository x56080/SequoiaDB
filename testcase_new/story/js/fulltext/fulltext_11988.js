/***************************************************************************
@Description :seqDB-11988 :hash切分表加入域使用自动切分，创建/删除全文索引 
@Modify list :
              2018-11-02  YinZhen  Create
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

   var clName = COMMCLNAME + "_ES_11988";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "hash", ShardingKey: { a: 1 }, AutoSplit: true } );

   //插入数据，数据分布覆盖：1个组、多个组上
   var records = new Array();
   for( var i = 0; i < 1; i++ )
   {
      var record = { a: "a" + i, b: "b" + i };
      records.push( record );
   }
   dbcl.insert( records );

   //数据分布覆盖：1个组，索引字段覆盖：非分区键
   commCreateIndex( dbcl, "fullIndex1_11988", { b: "text" } );
   commCheckIndexConsistency( dbcl, "fullIndex1_11988", true );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex1_11988", 1 );

   var dbOperator = new DBOperator();
   var cappedCL = dbOperator.getCappedCLs( COMMCSNAME, clName, "fullIndex1_11988" );
   var cappedCL = cappedCL[0];
   var count = cappedCL.count();
   if( count != 0 )
   {
      throw new Error( "expect record num:0, actual record num: " + count );
   }

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex1_11988" );
   dropIndex( dbcl, "fullIndex1_11988" );
   commCheckIndexConsistency( dbcl, "fullIndex1_11988", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================================One Group Not on ShardingKey================================" );

   //数据分布覆盖：1个组，索引字段覆盖：分区键
   commCreateIndex( dbcl, "fullIndex2_11988", { a: "text" } );
   commCheckIndexConsistency( dbcl, "fullIndex2_11988", true );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex2_11988", 1 );

   var cappedCL = dbOperator.getCappedCLs( COMMCSNAME, clName, "fullIndex2_11988" );
   var cappedCL = cappedCL[0];
   var count = cappedCL.count();
   if( count != 0 )
   {
      throw new Error( "expect record num:0, actual record num: " + count );
   }

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex2_11988" );
   dropIndex( dbcl, "fullIndex2_11988" );
   commCheckIndexConsistency( dbcl, "fullIndex2_11988", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================================One Group on ShardingKey================================" );

   var records = new Array();
   for( var i = 0; i < 9999; i++ )
   {
      var record = { a: "a" + i, b: "b" + i };
      records.push( record );
   }
   dbcl.insert( records );

   //数据分布覆盖：多个组，索引字段覆盖：非分区键
   commCreateIndex( dbcl, "fullIndex3_11988", { b: "text" } );
   commCheckIndexConsistency( dbcl, "fullIndex3_11988", true );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex3_11988", 10000 );

   var cappedCL = dbOperator.getCappedCLs( COMMCSNAME, clName, "fullIndex3_11988" );
   var cappedCL = cappedCL[0];
   var count = cappedCL.count();
   if( count != 0 )
   {
      throw new Error( "expect record num:0, actual record num: " + count );
   }

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex3_11988" );
   dropIndex( dbcl, "fullIndex3_11988" );
   commCheckIndexConsistency( dbcl, "fullIndex3_11988", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================================Many Group Not on ShardingKey================================" );

   //数据分布覆盖：多个组，索引字段覆盖：分区键
   commCreateIndex( dbcl, "fullIndex4_11988", { a: "text" } );
   commCheckIndexConsistency( dbcl, "fullIndex4_11988", true );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex4_11988", 10000 );

   var cappedCL = dbOperator.getCappedCLs( COMMCSNAME, clName, "fullIndex4_11988" );
   var cappedCL = cappedCL[0];
   var count = cappedCL.count();
   if( count != 0 )
   {
      throw new Error( "expect record num:0, actual record num: " + count );
   }

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex4_11988" );
   dropIndex( dbcl, "fullIndex4_11988" );
   commCheckIndexConsistency( dbcl, "fullIndex4_11988", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );
   println( "================================Many Group on ShardingKey================================" );

   dropCL( db, COMMCSNAME, clName, true, true );
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

