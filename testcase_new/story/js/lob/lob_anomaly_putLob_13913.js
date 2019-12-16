/******************************************************************************
*@Description : anomaly test that for putLob/listLob/deleteLob
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   var testFile = CHANGEDPREFIX + "lobTest.file";
   //lobGenerateFile( testFile ); // auto file
   // create collection
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true,
      "create collection" );
   // put lob with no lob file[Test_Point_1]
   try
   {
      var list = cl.listLobs( testFile );
      throw "ErrExecuteLob";
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "failed to execute listLobs with a parmeter, rc = " + e );
         throw e;
      }
      else
         println( "success to execute putLob with no lob file" );
   }

   try
   {
      cl.putLob();
      throw "ErrExecuteLob";
   }
   catch( e )
   {
      if( -259 != e )
      {
         println( "failed to execute put lob with no file, rc = " + e );
         throw e;
      }
      else
         println( "success to execute putLob with no lob file" );
   }

   // delete lob with no oid[Test_Point_2]
   try
   {
      // delete lobs
      cl.deleteLob();
      throw "ErrorDeleteLob";
   }
   catch( e )
   {
      if( -259 != e )
      {
         println( "failed to execute delete lob with no file, rc = " + e );
         throw e;
      }
      else
      {
         println( "success to execute deleteLob with no parameter, rc = " );
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
