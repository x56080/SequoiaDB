/******************************************************************************
@Description :   seqDB-15995:  创建集合时，创建自增字段为嵌套字段  
@Modify list :   2018-10-18    xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_15995";

   commDropCL( db, COMMCSNAME, clName );

   try
   {
      db.getCS( COMMCSNAME ).createCL( clName, { AutoIncrement: { Field: "a.1" } } );
      throw "create autoIncrement error!";
   } catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a.aa" } } );

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a.aa.aa" } } );

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
