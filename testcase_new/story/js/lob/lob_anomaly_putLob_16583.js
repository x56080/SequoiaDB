/******************************************************************************
*@Description: test putLob with incorrect oid
*@author:      wangkexin
*@createdate:  2018.11.20
*@testlinkCase: seqDB-16583:putLob, oid参数取值格式校验
******************************************************************************/

main( db );
function main ( db )
{
   var clName = CHANGEDPREFIX + "_putlob16583";
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );

   var testFile = CHANGEDPREFIX + "_lobTest16583.file";
   lobGenerateFile( testFile );

   var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, true, "create collection" );

   println( "begin to put lob with incorrect oid" )
   // put lob with incorrect oid
   try
   {
      var incorrectOid = "123456";
      cl.putLob( testFile, incorrectOid );
      throw "ErrExecuteLob";
   }
   catch( e )
   {
      if( -6 != e )
      {
         throw buildException( "check put lob with incorrect oid ", e );
      }
   }
   finally
   {
      var cmd = new Cmd();
      // remove lobfile
      cmd.run( "rm -rf " + testFile );
      commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the end, error" );
   }
}

