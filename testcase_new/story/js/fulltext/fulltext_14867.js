/************************************
*@Description: full text sort
*@author:      zhaoyu
*@createdate:  2018.10.12
**************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy mode is standalone!" );
      return;
   }

   var clName = COMMCLNAME + "_ES_14867";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_14867";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( { a: "text" } );

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 1 );

   //not support full text sort
   try
   {
      var cursor = dbcl.find( { "": { $Text: { query: { match_all: {} }, sort: [{ a: { order: "desc" } }] } } } );
      while( cursor.next() ) { }
      throw new Error( "NEED_SORT_ERR" );
   } catch( e )
   {
      if( e.message != -6 )
      {
         throw e;
      }
   }

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

