/************************************
*@Description: insert record with 16M
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

   var clName = COMMCLNAME + "_ES_12000";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_12000";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var str = new Array( 1024 * 1024 * 15 ).join( "a" );
   dbcl.insert( { a: str } );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( { a: str } );

   //check count,but not check record(out of memery)
   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 2 );
   println( "---check insert success---" );

   var str = new Array( 1024 * 1024 * 16 ).join( "a" );
   try
   {
      dbcl.insert( { a: str } );
      throw new Error( "NEED_INSERT_ERR" );
   } catch( e )
   {
      if( e.message != -24 )
      {
         throw e;
      }
   }

   checkFullSyncToES( COMMCSNAME, clName, indexName, 2 );

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

