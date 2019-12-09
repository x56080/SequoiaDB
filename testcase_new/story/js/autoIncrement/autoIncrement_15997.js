/******************************************************************************
@Description :   seqDB-15997:  创建集合时，创建字段名为多个字段的自增字段 
@Modify list :   2018-10-18    xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_15997";

   commDropCL( db, COMMCSNAME, clName );

   try
   {
      db.getCS( COMMCSNAME ).createCL( clName, { AutoIncrement: { Field: ["a", "b"] } } );
      throw "create autoIncrement error!";
   } catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }
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
