/******************************************************************************
@Description :   seqDB-16333:  删除不存在的自增字段 
@Modify list :   2018-11-12    xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_16333";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a1" } } );

   try
   {
      dbcl.dropAutoIncrement( "b1" );
      throw new Error( "drop error!" );
   } catch( e )
   {
      if( e !== -333 )
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
