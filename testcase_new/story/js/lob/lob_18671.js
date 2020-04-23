/******************************************************************************
*@Description : seqDB-18671:节点间lob一致性检查（lob、LSN）
*@Modify list :
*     2019-07-12 xiaoni huang init
******************************************************************************/
main();

function main ()
{
   try
   {
      if( commIsStandalone( db ) )
      {
         println( "The mode is standalone." );
         return;
      }

      if( commGetGroupsNum( db ) < 2 )
      {
         println( "The number of groups is less than 2." );
         return;
      }

      var csName = "cs18671";
      var clName1 = "cl1";
      var clName2 = "cl2";
      var pubLobFile = "lob18671";
      var getLobFile1 = pubLobFile + "_g1";
      var getLobFile2 = pubLobFile + "_g2";

      var cmd = new Cmd();
      var pwd = cmd.run( "pwd" ).split( "\n" )[0];
      println( "\nlocalDir = " + pwd );
      cmd.run( "rm -rf " + pubLobFile + "*" );
      lobGenerateFile( pubLobFile );

      println( "\n---Begin to get groups." );
      var groups = commGetGroups( db );
      var srcGroup = groups[0][0]["GroupName"];
      var dstGroup = groups[1][0]["GroupName"];

      println( "\n---Begin to ready cs / cl." );
      commDropCS( db, csName, true, "Failed to drop cs in the pre-condition." );
      var cs = db.createCS( csName );
      var cl1 = cs.createCL( clName1, { Group: srcGroup, ReplSize: 0 } );
      var cl2 = cs.createCL( clName2, { Group: srcGroup, ShardingType: "hash", ShardingKey: { a: 1 }, ReplSize: 0 } );

      println( "\n---Begin to cl1.putLob." );
      var putLobMd5 = cmd.run( "md5sum " + pubLobFile ).split( " " )[0];
      var lobOid = cl1.putLob( pubLobFile );

      println( "\n---Begin to cl2.split[" + srcGroup + ", " + dstGroup + "]." );
      cl2.split( srcGroup, dstGroup, 50 );

      println( "   Begin to cl2.truncate." );
      cl2.truncate();

      println( "\n---Begin to db.sync." );
      db.sync( { CollectionSpace: csName } );

      println( "\n---Begin to check results." );
      checkLobContent( csName, clName1, srcGroup, lobOid, cmd, putLobMd5, getLobFile1, getLobFile2 );
      checkLobLSN( csName, clName1, srcGroup );
      checkLobLSN( csName, clName2, srcGroup );
      checkLobLSN( csName, clName2, dstGroup );

      println( "\n---Begin to drop cs." );
      commDropCS( db, csName, false, "Failed to drop cs in the end-condition." );
      cmd.run( "rm -rf " + pubLobFile + "*" );
   }
   catch( e )
   {
      throw e;
   }
}

function checkLobContent ( csName, clName, groupName, lobOid, cmd, putLobMd5, getLobFile1, getLobFile2 )
{
   println( "   Begin to check lob content." );
   var rg = db.getRG( groupName );
   var sDB;
   var mDB;
   try
   {
      sDB = rg.getSlave().connect();
      mDB = rg.getMaster().connect();
      var sCL = sDB.getCS( csName ).getCL( clName );
      var mCL = mDB.getCS( csName ).getCL( clName );
      sCL.getLob( lobOid, getLobFile1 );
      mCL.getLob( lobOid, getLobFile2 );
   }
   finally
   {
      sDB.close();
      mDB.close();
   }

   var getLob1Md5 = cmd.run( "md5sum " + getLobFile1 ).split( " " )[0];
   var getLob2Md5 = cmd.run( "md5sum " + getLobFile2 ).split( " " )[0];
   if( putLobMd5 !== getLob1Md5 || getLob1Md5 !== getLob2Md5 )
   {
      throw buildException( "main", null, "[check lob content]",
         "[putLobMd5  = " + putLobMd5 + "]",
         "[getLob1Md5 = " + getLob1Md5 + ", getLob2Md5 = " + getLob2Md5 + "]" );
   }
}

function checkLobLSN ( csName, clName, groupName )
{
   var fullCLName = csName + "." + clName;
   println( "   Begin to check lob lsn, cl = " + fullCLName + ", rg = " + groupName );
   var rg = db.getRG( groupName );
   var sDB;
   var mDB;
   try
   {
      sDB = rg.getSlave().connect();
      mDB = rg.getMaster().connect();
      var sLSN = sDB.snapshot( SDB_SNAP_COLLECTIONS, { Name: fullCLName } ).current().toObj().Details[0].LobCommitLSN;
      var mLSN = mDB.snapshot( SDB_SNAP_COLLECTIONS, { Name: fullCLName } ).current().toObj().Details[0].LobCommitLSN;
      if( sLSN !== mLSN )
      {
         throw buildException( "main", null, "[check lob lsn]",
            "[sLSN = " + sLSN + "]",
            "[mLSN = " + mLSN + "]" );
      }
   }
   finally
   {
      sDB.close();
      mDB.close();
   }
}
