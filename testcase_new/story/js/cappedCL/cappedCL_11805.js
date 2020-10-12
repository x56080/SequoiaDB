/************************************
*@Description:capped cl findandUpdate/findandRemove
*@author:      zhaoyu
*@createdate:  2017.7.11
*@testlinkCase: seqDB-11805
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_11805";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   assert.tryThrow( -279, function()
   {
      dbcl.find().update( { $set: { a: 1 } } ).toArray();
   } );

   assert.tryThrow( -279, function()
   {
      dbcl.find().remove().toArray();
   } );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}