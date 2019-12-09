/******************************************************************************
*@Description : test function:
*               db.collectionspace.collection.getLob( < oid >, < file >, [forced] )
*               db.collectionspace.collection.putLob( < lobfile > )
*               db.collectionspace.collection.listLobs()
*               db.collectionspace.collection.deleteLob( < oid > )
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   var testFile = CHANGEDPREFIX + "_lobTest.file";
   var getTestFile = CHANGEDPREFIX + "_lobTestGet.file";
   var putNum = 10;
   var cmd = new Cmd();
   var oids = [];

   lobGenerateFile( testFile ); // auto file
   // cmd.run( "cat " + testFile ); 
   // create collection
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, -1, true, true, true,
      "create collection in the beginning" );
   var md5Arr = cmd.run( "md5sum " + testFile ).split( " " );
   var md5 = md5Arr[0];
   // put Lob
   try
   {
      println( "begin to put lob" );
      for( var i = 0; i < putNum; ++i )
      {
         oids.push( cl.putLob( testFile ) );
      }
      println( "put lob over" );
      // verify
      var cursor = cl.listLobs().toArray();
      if( putNum != cursor.length )
      {
         println( "collection have lob: " + cursor.length );
         throw "ErrNumberPutLob";
      }
      println( "success to put lob in colleciton" );
   }
   catch( e )
   {
      println( "failed to put lob in collection, rc = " + e );
      throw e;
   }
   // get lob
   try
   {
      for( var i = 0; i < oids.length; ++i )
      {
         cl.getLob( oids[i], getTestFile, true );
         md5Arr = cmd.run( "md5sum " + getTestFile ).split( " " );
         getMd5 = md5Arr[0];
         if( getMd5 !== md5 )// verify put file is equal get file or not
         {
            println( "put lob file md5: " + md5 );
            println( "get lob file md5: " + getMd5 );
            throw "NotEqualMd5";
         }
      }
      println( "success to get lob in colleciton" );
      // delete lobs
      for( var i = 0; i < oids.length; ++i )
      {
         cl.deleteLob( oids[i] );
      }
      println( "success to delete lob in colleciton" );
      // remove lobfile
      //cmd.run( "rm -rf " + testFile ); 
      //cmd.run( "rm -rf " + getTestFile ); 
   }
   catch( e )
   {
      // remove lobfile
      //cmd.run( "rm -rf " + testFile ); 
      println( "failed to get lob, rc = " + e );
      throw e;
   }
   finally
   {
      cmd.run( "rm -rf " + testFile );
      cmd.run( "rm -rf " + getTestFile );
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
