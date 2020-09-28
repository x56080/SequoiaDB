/************************************
*@Description: createCL，name长度无效_st.verify.CL.010
*@author:      wangkexin
*@createDate:  2019.6.6
*@testlinkCase: seqDB-4539
**************************************/
main( test );
function test ()
{
   //128B
   var clName = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890test4539";
   assert.tryThrow( -259, function()
   {
      db.getCS( COMMCSNAME ).createCL();
   } );

   assert.tryThrow( -6, function()
   {
      db.getCS( COMMCSNAME ).createCL( "" );
   } );

   assert.tryThrow( -6, function()
   {
      db.getCS( COMMCSNAME ).createCL( clName );
   } );

}
