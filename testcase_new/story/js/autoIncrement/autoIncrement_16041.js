/******************************************************************************
@Description :   seqDB-16041:  Cycled字段参数校验 
@Modify list :   2018-10-25    xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_16041";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a0" } } );

   var expRecs = [];
   for( var i = 0; i < 2; i++ )
   {
      dbcl.insert( { "a": i } );
      expRecs.push( { "a": i, "a0": i + 1 } );
   }

   var rc = dbcl.find();
   checkRec( rc, expRecs );

   //alter Cycled true
   dbcl.setAttributes( { AutoIncrement: { Field: "a0", Cycled: true, MaxValue: 1001, Increment: 1000, CacheSize: 1, AcquireSize: 1 } } );

   for( var i = 0; i < 2; i++ )
   {
      dbcl.insert( { "a": i } );
      expRecs.push( { "a": i, "a0": i * 1000 + 1 } );
   }

   var rc = dbcl.find();
   checkRec( rc, expRecs );

   //alter Cycled false
   dbcl.setAttributes( { AutoIncrement: { Field: "a0", Cycled: false, MaxValue: 1001, Increment: 1000, CacheSize: 1, AcquireSize: 1 } } );

   try
   {
      dbcl.insert( { "a": i } );
      throw new Error( "inert error!" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }

   var rc = dbcl.find();
   checkRec( rc, expRecs );

   //alter Cycled string
   try
   {
      dbcl.setAttributes( { Field: "a0", Cycled: "cycled" } );
      throw new Error( "create error!" );
   } catch( e )
   {
      if( e !== -6 )
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
