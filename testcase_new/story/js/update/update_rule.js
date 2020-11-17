// update record.
// unnormal rule. 

main( test );
function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );

   varCL.insert( { a: [1, 2], salary: 100 } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.update( { $addtoset: { b: 2 } } );
   } );

   varCL.update( { $pull: { "a.0": 1 } } );

   varCL.update( { $push: { salary: 1 } } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.update( { $pull_all: { a: 3 } } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.update( { $push_all: { a: 2 } } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.update( { $pop: { a: [2] } } );
   } );
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
}