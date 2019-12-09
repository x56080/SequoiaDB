/******************************************************************************
*@Description : test same collection input nomal record and lob data
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   var testFile = CHANGEDPREFIX + "lobTest.file";
   var getTestFile = CHANGEDPREFIX + "lobTestGet.file";
   var putNum = 50;

   lobGenerateFile( testFile ); // auto file
   var originMd5 = getMd5ForFile( testFile );

   // create collection
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, -1, true, true, true,
      "create collection" );
   // put normal data and put lob
   try
   {
      oid = lobPutLob( cl, testFile, putNum );
      println( "success to put lob" );
      lobInsertDoc( cl, putNum );
      println( "success to insert normal record" );

      for( var i = 0; i < oid.length; ++i )
      {
         cl.getLob( oid[i], getTestFile, true );
         var curMd5 = getMd5ForFile( getTestFile );
         if( originMd5 !== curMd5 )
         {
            throw "origin file's md5=" + originMd5 + "getLob's md5=" + curMd5;
         }
      }
      println( "success to get lob data" );
      for( var i = 0; i < cl.count(); ++i )
      {
         var count = cl.find( { "no": i } ).count();
         if( 1 != count )
         {
            println( "failed to query data, rc = " + cl.find( { "no": i } ) );
            throw "ErrNumberQuery";
         }
      }
      println( "success to query data" );

   }
   catch( e )
   {
      println( "failed to put lob and normal record in collection, rc = " + e );
      throw e;
   }
   finally
   {
      var cmd = new Cmd();
      cmd.run( "rm -rf " + testFile );
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile );
      }
   }
}

// Run Main
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clear collection in the beginning" );
   main( db );
}
catch( e )
{
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop collection in the end, error" );
   db.close();
}
