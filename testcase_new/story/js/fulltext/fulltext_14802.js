/************************************
*@Description: no fullText index,query with fullText
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

   var clName = COMMCLNAME + "_ES_14802";
   var clFullName = COMMCSNAME + "." + clName;

   commDropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   dbcl.insert( { a: "text" } );

   try
   {
      var cursor = dbcl.find( { "": { $Text: { query: { match_all: {} } } } } );
      while( cursor.next() ) { }
      throw new Error( "NEED_FIND_ERR" );
   } catch( e )
   {
      if( e !== -52 )
      {
         throw new Error( e );
      }
   }

   commDropCL( db, COMMCSNAME, clName );
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
