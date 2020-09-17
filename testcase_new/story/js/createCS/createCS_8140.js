// create cs.
// unnormal_2 case.

main( test );
function test ()
{
   var csName = COMMCSNAME + "_._8140";

   assert.tryThrow( -6, function()
   {
      db.createCS( csName );
   } );
}