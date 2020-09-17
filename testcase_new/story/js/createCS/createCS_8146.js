// create cs.
// unnormal_1 case.
main( test );
function test ()
{
   var csName = "SYS" + "_8146";

   assert.tryThrow( -6, function()
   {
      db.createCS( csName );
   } );

}