/******************************************************************************
*@Description : abnormal parameter verification for getLob().
*@Modify list :
*               2019-05-29  wuyan  Init
******************************************************************************/
main( db );
function main ( db )
{
   try
   {
      var clName = "testLob4468";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );

      var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "create collection" );
      var lobFilePath = WORKDIR + "/testlob4468";
      lobGenerateFile( lobFilePath );
      var lobOid = cl.putLob( lobFilePath );

      //test case:4468
      getLobWithOidNotExist( cl );
      //test case:4475
      getLobWithIllegalForced( cl, lobOid );
      //test case:4476
      getLobWithEmptyForced( cl, lobOid )

      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "clear collection in the ending" );
   }
   finally
   {
      var cmd = new Cmd();
      // remove lobfile
      cmd.run( "rm -rf " + lobFilePath );
   }


}

function getLobWithOidNotExist ( cl )
{
   println( "---begin to test testcase-4468" );
   try
   {
      var getFilePath = WORKDIR + "/getlob4468";
      var lobOid = "5ce6016a97216ce21b5c982a";
      cl.getLob( lobOid, getFilePath );
      throw "get Lob with oid not exist must be fail!";
   }
   catch( e )
   {
      //error:Forced must be bool -4
      if( -4 !== e )
      {
         throw buildException( "getLobWithOidNotExist", e );
      }
   }
}

function getLobWithIllegalForced ( cl, lobOid )
{
   println( "---begin to test testcase-4475" );
   try
   {
      var getFilePath = WORKDIR + "/getlob4475";
      var illegalForced = "test";
      cl.getLob( lobOid, getFilePath, illegalForced );
      throw "get Lob with illegal forced must be fail!";
   }
   catch( e )
   {
      //error:Forced must be bool -6
      if( -6 !== e )
      {
         throw buildException( "deleteLobWithEmpty", e );
      }
   }
}

function getLobWithEmptyForced ( cl, lobOid )
{
   println( "---begin to test testcase-4476" );
   try
   {
      var getFilePath = WORKDIR + "/getlob4476";
      var forced = null;
      cl.getLob( lobOid, getFilePath, null );
      throw "get Lob with empty forced must be fail!";
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw buildException( "getLobWithEmptyForced", e );
      }
   }
}

