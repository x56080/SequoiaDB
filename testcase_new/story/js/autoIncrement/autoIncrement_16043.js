/******************************************************************************
@Description :   seqDB-16043:  CurrentValue字段参数校验 
@Modify list :   2018-12-25    xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_16043";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName );
   dbcl.createAutoIncrement( { Field: "a" } );

   dbcl.setAttributes( { AutoIncrement: { Field: "a" } } );
   var clID = getCLID( COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_a_SEQ";
   var expSequence = {};
   checkSequence( sequenceName, expSequence );
   var expRecs = [];
   for( var i = 0; i < 100; i++ )
   {
      dbcl.insert( { "q": i } );
      expRecs.push( { "q": i, "a": 1 + i } );
   }
   var rc = dbcl.find();
   checkRec( rc, expRecs );

   dbcl.setAttributes( { AutoIncrement: { Field: "a", CurrentValue: { "$numberLong": "9223372036854775807" } } } );
   var expSequence = { CurrentValue: { "$numberLong": "9223372036854775807" } };
   checkSequence( sequenceName, expSequence );
   try
   {
      dbcl.insert( { "q": 2 } );
      throw new Error( "insert ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   var rc = dbcl.find();
   checkRec( rc, expRecs );

   dbcl.setAttributes( { AutoIncrement: { Field: "a", CurrentValue: 4 } } );
   for( var i = 0; i < 100; i++ )
   {
      dbcl.insert( { "q": i } );
      expRecs.push( { "q": i, "a": 5 + i } );
   }
   var rc = dbcl.find();
   checkRec( rc, expRecs );

   dbcl.setAttributes( { AutoIncrement: { Field: "a", CurrentValue: { "$numberLong": "9223372036854775809" } } } );
   var expSequence = { CurrentValue: { "$numberLong": "9223372036854775807" } };
   checkSequence( sequenceName, expSequence );
   try
   {
      dbcl.insert( { "q": 2 } );
      throw new Error( "insert ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   var rc = dbcl.find();
   checkRec( rc, expRecs );

   dbcl.dropAutoIncrement( "a" );
   dbcl.createAutoIncrement( { Field: "a", Increment: -1 } );
   dbcl.setAttributes( { AutoIncrement: { Field: "a", CurrentValue: { "$numberLong": "-9223372036854775808" } } } );
   var expSequence = { CurrentValue: { "$numberLong": "-9223372036854775808" }, Increment: -1, MinValue: { "$numberLong": "-9223372036854775808" }, MaxValue: -1, StartValue: -1 };
   checkSequence( sequenceName, expSequence );
   try
   {
      dbcl.insert( { "q": 2 } );
      throw new Error( "insert ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   var rc = dbcl.find();
   checkRec( rc, expRecs );

   dbcl.setAttributes( { AutoIncrement: { Field: "a", CurrentValue: { "$numberLong": "-9223372036854775809" } } } );
   var expSequence = { CurrentValue: { "$numberLong": "-9223372036854775808" }, Increment: -1, MinValue: { "$numberLong": "-9223372036854775808" }, MaxValue: -1, StartValue: -1 };
   checkSequence( sequenceName, expSequence );
   try
   {
      dbcl.insert( { "q": 2 } );
      throw new Error( "insert ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   var rc = dbcl.find();
   checkRec( rc, expRecs );

   try
   {
      dbcl.setAttributes( { AutoIncrement: { Field: "a", CurrentValue: "a" } } );
      throw new Error( "setAttributes error!" );
   } catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }

   try
   {
      dbcl.insert( { "q": 2 } );
      throw new Error( "insert ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   var rc = dbcl.find();
   checkRec( rc, expRecs );

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
