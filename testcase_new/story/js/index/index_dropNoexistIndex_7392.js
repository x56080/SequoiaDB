/***************************************************************************
@Description : drop the index ,but index don't exist.
@Modify list :
               2014-5-16  xiaojun Hu  modify
***************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
   var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   assert.tryThrow( -47, function()
   {
      varCL.dropIndex( "testindex" );
   } );

   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false );

}

