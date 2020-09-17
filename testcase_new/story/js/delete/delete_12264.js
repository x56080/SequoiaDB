// delete record.
// normal case.

main( test );
function test ()
{
   var clName = COMMCLNAME + "_12264";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the beginning" );

   var varCL = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "create collecton 1 failed" );

   varCL.remove();
   var cursor = varCL.find();
   commCompareResults( cursor, [] );
   commDropCL( db, COMMCSNAME, clName, false, false, "drop colleciton in the end" );
}
