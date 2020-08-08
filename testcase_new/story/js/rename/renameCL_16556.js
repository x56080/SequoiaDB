/************************************
*@Description: 修改cl名，新名参数校验
*@author:      luweikang
*@createdate:  2018.12.13
*@testlinkCase:seqDB-16556
**************************************/

main();

function main ()
{
   var csName = COMMCSNAME + "_16556";
   var clName = "cl_16656";

   commCreateCL( db, csName, clName );

   // rename cs new name is begin with $
   checkNewCLName( db, csName, clName, "$clName16556", -6 );

   // rename cs new name is contains .
   checkNewCLName( db, csName, clName, "clName.16556", -6 );

   // rename cs new name is ""
   checkNewCLName( db, csName, clName, "", -6 );

   // rename cs new name is long str
   var longStr = "a";
   for( var i = 0; i < 1000; i++ )
   {
      longStr += "a";
   }
   checkNewCLName( db, csName, clName, longStr, -6 );

   // rename cs new name is 128 str
   var boundStr = "";
   for( var i = 0; i < 128; i++ )
   {
      boundStr += "s";
   }
   checkNewCLName( db, csName, clName, boundStr, -6 );

   // rename cs new name is 127 str
   var shotStr = "";
   for( var i = 0; i < 127; i++ )
   {
      shotStr += "a";
   }
   checkNewCLName( db, csName, clName, shotStr, 0 );
   db.getCS( csName ).renameCL( shotStr, clName );

   // rename cs new name is contains ~!@#$%^()_+
   var nameStr = "~!@#$%^()_+"
   checkNewCLName( db, csName, clName, nameStr, 0 );
   db.getCS( csName ).renameCL( nameStr, clName );

   // rename cs new name is begin with SYS
   checkNewCLName( db, csName, clName, "SYScsName16556", -6 );

   commDropCS( db, csName, true, "clean cs---" );

}

function checkNewCLName ( db, csName, oldCLName, newCLName, error )
{
   try
   {
      db.getCS( csName ).renameCL( oldCLName, newCLName );
      if( error !== 0 )
      {
         throw buildException( "rename cl new name is error, exp error: " + error );
      }
   }
   catch( e )
   {
      if( e != error )
      {
         throw buildException( "rename cl new name is error:" + newCLName, e, "rename", error, e );
      }
   }
}

