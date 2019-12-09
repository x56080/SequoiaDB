/************************************
*@Description: bulk insert
*@author:      zhaoyu
*@createdate:  2018.9.28
**************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy mode is standalone!" );
      return;
   }

   var clName = COMMCLNAME + "_ES_12001";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_12001";

   commDropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   for( var j = 0; j < 2; j++ )
   {
      var doc = [];
      for( var i = 0; i < 10000; i++ )
      {
         doc.push( { a: j * 10000 + i + "text" } );
      }
      dbcl.insert( doc );
   }
   commCreateIndex( dbcl, indexName, { a: "text" } );

   for( var j = 0; j < 2; j++ )
   {
      var doc = [];
      for( var i = 0; i < 10000; i++ )
      {
         doc.push( { a: j * 10000 + i + "text" } );
      }
      dbcl.insert( doc );
   }

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 40000 );

   var expectRecords = dbOperator.findFromCL( dbcl, null, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );
   println( "---check insert success---" );

   dbcl.update( { $set: { a: "update" } } );
   dbcl.insert( { a: "update" } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 40001 );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: "update" }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match: { a: "update" } } } } }, null, { _id: 1 } );
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

