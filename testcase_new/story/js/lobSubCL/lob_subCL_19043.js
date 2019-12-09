/************************************
*@Description: seqDB-19043 主表对lob执行truncate
*@author:      luweikang
*@createDate:  2019.8.12
**************************************/
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "skip standalone mode" );
      return;
   }
   var csName = COMMCSNAME;
   var mainCLName = "cl19043_main";
   var subCLName = "cl19043_sub";
   var filePath = WORKDIR + "/lob19043/";
   var fileName1 = "file19043_1";
   var fileName2 = "file19043_2";
   var fileName3 = "file19043_3";
   var fileSize = 1024 * 1024;
   var file1MD5 = makeTmpFile( filePath, fileName1, fileSize );
   var file2MD5 = makeTmpFile( filePath, fileName2, fileSize );
   var file3MD5 = makeTmpFile( filePath, fileName3, fileSize );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCLByOption( db, csName, mainCLName, options, true, false, "create main cl" );
   commCreateCL( db, csName, subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": { "$minKey": 1 } }, "UpBound": { "date": { "$maxKey": 1 } } } );

   var lobID1 = mainCL.putLob( filePath + fileName1 );
   var lobID2 = mainCL.putLob( filePath + fileName2 );
   var lobID3 = mainCL.putLob( filePath + fileName3 );

   //truncate length = 0
   println( "---truncate lob length to 0---" );
   mainCL.truncateLob( lobID1, 0 );
   var lobInfo = mainCL.getLob( lobID1, filePath + "check19043_1" );
   var actFileSize = File.getSize( filePath + "check19043_1" );
   var actLobSize = lobInfo.toObj().LobSize;
   if( actLobSize !== 0 )
   {
      throw buildException( "check truncateLob1", null, "check read lob file size, lobId: " + lobID1, 0, actLobSize );
   }
   if( actFileSize !== 0 )
   {
      throw buildException( "check truncateLob1", null, "check read lob file size, lobId: " + lobID1, 0, actFileSize );
   }

   //truncate length = fileSize /2
   println( "---truncate lob length to fileSize / 2---" );
   var length = fileSize / 2;
   var cmd = new Cmd();
   mainCL.truncateLob( lobID2, length );
   cmd.run( "truncate -s " + ( length ) + " " + filePath + fileName2 );
   var lobInfo = mainCL.getLob( lobID2, filePath + "check19043_2" );
   var actLobSize = lobInfo.toObj().LobSize;
   var expMD5 = File.md5( filePath + fileName2 );
   var actMD5 = File.md5( filePath + "check19043_2" );
   if( actLobSize !== length )
   {
      throw buildException( "check truncateLob2", null, "check read lob file size, lobId: " + lobID2, length, actLobSize );
   }
   if( expMD5 !== actMD5 )
   {
      throw buildException( "check truncateLob2", null, "check read lob file md5, lobId: " + lobID2, expMD5, actMD5 );
   }

   //truncate length = fileSize
   println( "---truncate lob length to fileSize---" );
   mainCL.truncateLob( lobID3, fileSize );
   var lobInfo = mainCL.getLob( lobID3, filePath + "check19043_3" );
   var actLobSize = lobInfo.toObj().LobSize;
   var actMD5 = File.md5( filePath + "check19043_3" );
   if( actLobSize !== fileSize )
   {
      throw buildException( "check truncateLob3", null, "check read lob file size, lobId: " + lobID3, fileSize, actLobSize );
   }
   if( file3MD5 !== actMD5 )
   {
      throw buildException( "check truncateLob3", null, "check read lob file md5, lobId: " + lobID3, file3MD5, actMD5 );
   }

   cleanMainCL( db, csName, mainCLName );
   deleteTmpFile( filePath );
}

