/***************************************************************************
@Description :seqDB-14382 :shardingKey字段为全文索引字段，更新全文索引字段
@Modify list :2018-11-22  Zhaoxiaoni  init
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

   var clName = COMMCLNAME + "_ES_14382";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { ShardingType: "range", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, "fullIndex_14382", { a: "text" } );

   var records = [];
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: "a" + i, b: i };
      records.push( record );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_14382", 10000 );

   dbcl.update( { $set: { a: "a", b: "b" } } );
   try
   {
      dbcl.update( { $set: { a: "a", b: "bb" } }, null, null, { KeepShardingKey: true } );
   } catch( e )
   {
      if( e.message != -178 )
      {
         throw e;
      }
   }
   dbcl.insert( { a: "new" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_14382", 10001 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_14382" );
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
