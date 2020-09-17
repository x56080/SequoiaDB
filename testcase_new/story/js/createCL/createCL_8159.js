//creat cl
//innomal case2

main( test );
function test ()
{
   var clName = "." + "_8159";
   var cs = db.getCS( COMMCSNAME );
   assert.tryThrow( -6, function()
   {
      cs.createCL( clName );
   } );
}