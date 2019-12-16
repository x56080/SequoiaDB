/***************************************************************************
@Description :seqDB-12053 :range分区表中执行全文检索  
@Modify list :2018-11-21  Zhaoxiaoni  init
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

   var clName = COMMCLNAME + "_ES_12053";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "range", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, "fullIndex_12053", { a: "text", b: "text" } );

   var records = [];
   for( var i = 0; i < 5000; i++ )
   {
      var record1 = { a: "a", b: "i" };
      var record2 = { a: "f", b: "i" };
      records.push( record1 );
      records.push( record2 );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12053", 10000 );
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], { a: "c" }, { a: "g" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12053", 10000 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   println( "===find from one group success===" );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   println( "===find from more group success===" );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_12053" );
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