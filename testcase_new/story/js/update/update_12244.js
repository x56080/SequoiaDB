// update record.
// unnormal rule.

main( test );
function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

   // prepare env
   // commCreateCL( db, csName, clName, replSize, compressed, autoCreateCS, ignoreExisted, message )
   var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   varCL.insert( { a: [1, 2] } );

   varCL.update( { $set: { "a.c": 0 } } );

   checkResult( varCL, {}, [{ a: [1, 2] }] );
   varCL.insert( { a: { a: 3 } } );


   varCL.update( { $set: { "a.a": "b" } } );

   checkResult( varCL, { "a.a": "b" }, [{ "a": { "a": "b" } }] );


   varCL.insert( { a: 3 } );


   varCL.update( { $set: { "a": 0 } } );

   checkResult( varCL, { a: 0 }, [{ a: 0 }, { a: 0 }, { a: 0 }] )
   // clear env
   // commDropCL( db, csName, clName, ignoreCSNotExist, ignoreCLNotExist, message )
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "Clear env after usecase" );
}