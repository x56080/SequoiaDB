/************************************
*@Description: record not include text index field, insert/update/delete
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

   var clName = COMMCLNAME + "_ES_11996";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_11996";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.insert( { b: "text" } );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( { b: "text" } );

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0 );

   var expectRecords = dbOperator.findFromCL( dbcl, { a: { $type: 2, $et: "string" } } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );
   println( "---check insert success---" );

   dbcl.update( { $set: { b: "update" } } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0 );
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
   dropCL( db, COMMCSNAME, clName );
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

