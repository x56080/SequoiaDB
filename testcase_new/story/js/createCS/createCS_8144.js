// create cs.
// CSname's large is 127.
main( test );
function test ()
{
   var csName = COMMCSNAME + "_8144";

   var len = csName.length;
   for( var i = 0; i < 128 - len; i++ )
   {
      csName += "a";
   }

   assert.tryThrow( -6, function()
   {
      db.createCS( csName );
   } );
}