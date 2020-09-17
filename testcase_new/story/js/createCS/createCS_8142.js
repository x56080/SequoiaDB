// create cs
// unnormal case
main( test );
function test ()
{
   var csName = COMMCSNAME + "_8142";

   for( var i = 0; i < 10000; i++ )
   {
      csName += "a";
   }

   assert.tryThrow( -6, function()
   {
      db.createCS( csName );
   } );
}