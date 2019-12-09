/* *****************************************************************************
@discretion: step one :operator isnull update test
@modify list:
   2014-4-10 YiBang Ruan  Init
***************************************************************************** */

function isnull_update_Test ( db )
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop cl in the beginning" );
   var cs = commCreateCS( db, COMMCSNAME, true );
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, false, true );
   cl.insert( { a: null, b: 1 } );
   cl.insert( { a: 1, b: 2 } );

   // update
   cl.update( { $set: { b: 2 } }, { a: { $isnull: 1 } } );
   cl.update( { $set: { b: 1 } }, { a: { $isnull: 0 } } );

   record1 = cl.find( { a: { $isnull: 1 } } );
   record2 = cl.find( { a: { $isnull: 0 } } );

   e1 = eval( "(" + record1[0] + ")" );
   e2 = eval( "(" + record2[0] + ")" );

   // check the result of match {a:null,b:1}
   if( record1.count() != 1 )
   {
      println( " wrong match result, record1 count = " + record1.count() );
      throw -1;
   }
   else
   {
      if( e1["b"] != 2 )
      {
         println( " wrong match result, e1[0]['b'] != 1 , it is " + e1["b"] );
         throw -1;
      }
   }

   // check the result of match {a:1,b:2}
   if( record2.count() != 1 )
   {
      println( " wrong match result, record2 count = " + record2.count() );
      throw -1;
   }
   else
   {
      if( e2["b"] != 1 )
      {
         println( " wrong match result, e2[0]['b'] != 1 , it is " + e2["b"] );
         throw -1;
      }
   }
}

function main ( db )
{
   isnull_update_Test( db );
}

try
{
   main( db );
}
catch( e )
{
   println( "isnull_update_Test failed: " + e );
   throw e;
}

