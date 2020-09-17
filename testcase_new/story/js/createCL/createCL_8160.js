//creat cl
//innomal case3

main( test );
function test ()
{
   var clName = "";
   var cs = db.getCS( COMMCSNAME );
   assert.tryThrow( -6, function()
   {
      cs.createCL( clName );
   } );
}