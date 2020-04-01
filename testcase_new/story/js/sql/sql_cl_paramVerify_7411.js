/****************************************************
@description:	create/drop CL by SQL, verify parameters
         testlink cases:   seqDB-7411/7412
@input:        1 create/drop cl, the length of the name is 127B (valid length), success
               2 create/drop cl, the length of the name is 128B (invalid length), errorno: -6 
               3 create/drop cl, the name is " ", errorno: -6
               4 create/drop cl, the name cotains special characters, errorno: -6
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
testConf.csName = COMMCSNAME, testConf.csOpt = { PageSize: 4096 };

main( test );

function test ()
{
   clName = CHANGEDPREFIX + "_bar";

   try  //clean cl
   {
      db.execUpdate( "drop collection " + testConf.csName + "." + clName );
   }
   catch( e )
   {
      if( e.message != -23 )
      {
         throw new Error( "Failed to clean env in the begin." );
      }
   }

   var specCLName1 = "";
   for( var i = 0; i < 127; ++i )
   {
      specCLName1 = specCLName1 + "a";
   }

   try  //clean cl
   {
      db.execUpdate( "drop collection " + testConf.csName + "." + specCLName1 );
   }
   catch( e )
   {
      if( e.message != -23 )
      {
         throw new Error( "Failed to clean env " + testConf.csName + "." + specCLName1 );
      }
   }

   db.execUpdate( "create collection " + testConf.csName + "." + specCLName1 );

   try
   {
      db.getCS( testConf.csName ).getCL( specCLName1 );
   }
   catch( e )
   {
      if( e.message != -23 )
      {
         throw new Error( "Failed to drop cl. rc=3" );
      }
   }

   db.execUpdate( "drop collection " + testConf.csName + "." + specCLName1 );

   var specCLName2 = "";
   for( var i = 0; i < 128; ++i )
   {
      specCLName2 = specCLName2 + "a";
   }

   try
   {
      db.execUpdate( "create collection " + testConf.csName + "." + specCLName2 );
      db.execUpdate( "drop collection " + testConf.csName + "." + specCLName2 );
   }
   catch( e )
   {
      if( e.message != -6 )
      {
         throw new Error( "The length of the name is 128B (invalid length),create cl success. Except errorno: -6" );
      }
   }

   try
   {
      db.execUpdate( "create collection " + testConf.csName + "." + " " );
      db.execUpdate( "drop collection " + testConf.csName + "." + " " );
   }
   catch( e )
   {
      if( e.message != -6 )
      {
         throw new Error( e );
      }
   }

   var aa = Array( "$", ".", "a." );
   for( var i = 0; i < aa.length; ++i )
   {
      try
      {
         var specCLName3 = aa[i] + CHANGEDPREFIX;
         db.execUpdate( "create collection " + testConf.csName + "." + specCLName3 );
         db.execUpdate( "drop collection " + testConf.csName + "." + specCLName3 );
      }
      catch( e )
      {
         if( e.message != -6 )
         {
            throw new Error( e );
         }
      }
   }
}