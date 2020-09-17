// create cs.
// unnormal_3 case
main( test );
function test ()
{
   var csName = "";

   assert.tryThrow( -6, function()
   {
      db.createCS( csName );
   } );
}