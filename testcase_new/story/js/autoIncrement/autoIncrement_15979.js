/******************************************************************************
@Description :   seqDB-15979: 删除记录 
@Modify list :   2018-10-15  xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_15979";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1" } } );

   dbcl.insert( { a: 1, b: 1 } );

   dbcl.remove();

   while( dbcl.find().next() )
   {
      throw new Error( "remove error" );
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
