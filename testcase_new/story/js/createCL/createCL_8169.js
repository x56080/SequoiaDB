// create cl.
// normal case.

main( test );
function test ()
{
   var clName = CHANGEDPREFIX + "_8169";
   var cs = db.getCS( COMMCSNAME );

   assert.tryThrow( -6, function()
   {
      cs.createCL( "SYS" + clName );
   } );

}