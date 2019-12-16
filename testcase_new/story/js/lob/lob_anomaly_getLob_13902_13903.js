/******************************************************************************
*@Description : anomaly test for getLob
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   var testFile = CHANGEDPREFIX + "lobTest.file";
   var getTestFile = CHANGEDPREFIX + "lobTestGet.file";
   var putNum = 1;


   lobGenerateFile( testFile );
   // create collection
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true,
      "create collection" );
   // put Lob
   var oid = lobPutLob( cl, testFile, putNum ); //Array
   // get lob with no parmameter : getLob()[Test_Point_1]
   try
   {
      cl.getLob();
   }
   catch( e )
   {
      if( -259 != e )
      {
         println( "failed to execute get lob with no file, rc = " + e );
         throw e;
      }
      else
      {
         println( "success to execute getLob with no parameter, rc = " );
      }
   }

   // get lob specify same file and don't set forced : true[Test_Point_2]
   var isFirstOper = true;
   try
   {
      cl.getLob( oid[0], getTestFile );
      isFirstOper = false;
      cl.getLob( oid[0], getTestFile );
      throw "ErrorExecuteGetLob";
   }
   catch( e )
   {
      if( isFirstOper || -5 != e )
      {
         println( "failed to execute get lob with no parameter:" +
            " forced, rc = " + e );
         throw e;
      }
      else
      {
         println( "success to execute getLob with no parameter:'forced', rc = " );
      }
   }
   // delete lob
   try
   {
      // delete lobs
      for( var i = 0; i < oid.length; ++i )
      {
         cl.deleteLob( oid[i] );
      }
      println( "success to delete lob in colleciton" );

   }
   catch( e )
   {
      println( "failed to delete lob, rc = " + e );
      throw e;
   }
   finally
   {
      var cmd = new Cmd();
      // remove lobfile
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
