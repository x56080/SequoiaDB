/************************************
*@Description: record include part of text index field, insert/update/delete
*@author:      zhaoyu
*@createdate:  2018.10.11
**************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy mode is standalone!" );
      return;
   }

   var clName = COMMCLNAME + "_ES_11997";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "abc_11997";

   commDropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.insert( { a: "text", d: "text" } );
   commCreateIndex( dbcl, indexName, { a: "text", b: "text", c: "text" } );
   dbcl.insert( { a: "text", d: "text" } );

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 2 );

   var expectRecords = dbOperator.findFromCL( dbcl, { a: { $type: 2, $et: "string" } } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );
   println( "---check insert success---" );

   dbcl.update( { $set: { a: "update" } } );
   dbcl.insert( { a: "update", d: "update" } )
   checkFullSyncToES( COMMCSNAME, clName, indexName, 3 );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: { $type: 2, $et: "string" } } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );
   println( "---check update success---" );

   dbcl.remove();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0 );
   var expectRecords = dbOperator.findFromCL( dbcl );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );
   println( "---check remove success---" );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   commDropCL( db, COMMCSNAME, clName );
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

