/* *****************************************************************************
@discretion: step one :operator isnull update test2
@modify list:
   2014-4-10 YiBang Ruan  Init
***************************************************************************** */

function isnull_update_Test2 ( db )
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop cl in the beginning" );
   var cs = commCreateCS( db, COMMCSNAME, true );
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, false, true );
   cl.insert( { a: null, b: 1 } );
   cl.insert( { b: 2 } );

   // update
   cl.update( { $set: { b: 3 } }, { a: { $isnull: 1 } } );
   cl.update( { $set: { b: 1 } }, { a: { $isnull: 0 } } );

   record1 = cl.find( { b: { $et: 3 } } );
   record2 = cl.find( { b: { $et: 1 } } );

   e1 = eval( "(" + record1[0] + ")" );
   e2 = eval( "(" + record2[0] + ")" );

   // check the result of match {a:null,b:3}\{b:3}
   if( record1.count() != 2 )
   {
      println( " wrong match result, record1 count = " + record1.count() );
      throw -1;
   }

   // check the result of match nothing
   if( record2.count() != 0 )
   {
      println( " wrong match result, record2 count = " + record2.count() );
      throw -1;
   }
}

function main ( db )
{
   isnull_update_Test2( db );
}

try
{
   main( db );
}
catch( e )
{
   println( "isnull_update_Test2 failed: " + e );
   throw e;
}

