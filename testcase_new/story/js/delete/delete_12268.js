// delete record.
// normal case.

main( test );
function test ()
{
   var clName = COMMCLNAME + "_12268";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the beginning" );
   var varCL = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "create collecton 1 failed" );

   var docs = [];
   docs.push( { a: 1 } );
   docs.push( { b: [1, 2], salary: 10, name: "Tom" } );
   varCL.insert( docs );

   varCL.remove( { name: "Mike" } );
   var cursor = varCL.find();
   commCompareResults( cursor, [{ a: 1 }, { b: [1, 2], salary: 10, name: "Tom" }] );
   varCL.remove( { a: 1 } );

   rc = varCL.find( { a: 1 } );
   var cursor = varCL.find();
   commCompareResults( cursor, [{ b: [1, 2], salary: 10, name: "Tom" }] );
   commDropCL( db, COMMCSNAME, clName, false, false, "drop colleciton in the end" );
}