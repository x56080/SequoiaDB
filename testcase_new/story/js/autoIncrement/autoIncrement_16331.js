/******************************************************************************
@Description :   seqDB-16331:  创建已存在的自增字段  
@Modify list :   2018-11-12    xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_16331";

   commDropCL( db, COMMCSNAME, clName );

   try
   {
      db.getCS( COMMCSNAME ).createCL( clName, { AutoIncrement: [{ Field: "a1", Increment: 2 }, { Field: "a1", Increment: 3 }, { Field: "a1", Increment: 4 }] } );
      throw new Error( "create error!" );
   } catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }

   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a1", Increment: 2 } } );

   try
   {
      dbcl.createAutoIncrement( { Field: "a1" } );
      throw new Error( "create error!" );
   } catch( e )
   {
      if( e !== -332 )
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
