/************************************
*@Description: 修改cl名，新名参数校验
*@author:      luweikang
*@createdate:  2018.12.13
*@testlinkCase:seqDB-16556
**************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME + "_16556";
   var clName = "cl_16656";

   commCreateCL( db, csName, clName );

   // rename cs new name is begin with $
   assert.tryThrow( -6, function()
   {
      db.getCS( csName ).renameCL( clName, "$clName16556" );
   } );

   // rename cs new name is contains .
   assert.tryThrow( -6, function()
   {
      db.getCS( csName ).renameCL( clName, "clName.16556" );
   } );

   // rename cs new name is ""
   assert.tryThrow( -6, function()
   {
      db.getCS( csName ).renameCL( clName, "" );
   } );

   // rename cs new name is long str
   var longStr = "a";
   for( var i = 0; i < 1000; i++ )
   {
      longStr += "a";
   }
   assert.tryThrow( -6, function()
   {
      db.getCS( csName ).renameCL( clName, longStr );
   } );

   // rename cs new name is 128 str
   var boundStr = "";
   for( var i = 0; i < 128; i++ )
   {
      boundStr += "s";
   }
   assert.tryThrow( -6, function()
   {
      db.getCS( csName ).renameCL( clName, boundStr );
   } );

   // rename cs new name is 127 str
   var shotStr = "";
   for( var i = 0; i < 127; i++ )
   {
      shotStr += "a";
   }
   db.getCS( csName ).renameCL( clName, shotStr );
   db.getCS( csName ).renameCL( shotStr, clName );

   // rename cs new name is contains ~!@#$%^()_+
   var nameStr = "~!@#$%^()_+"
   db.getCS( csName ).renameCL( clName, nameStr );
   db.getCS( csName ).renameCL( nameStr, clName );

   // rename cs new name is begin with SYS
   assert.tryThrow( -6, function()
   {
      db.getCS( csName ).renameCL( clName, "SYScsName16556" );
   } );

   commDropCS( db, csName, true, "clean cs---" );
}