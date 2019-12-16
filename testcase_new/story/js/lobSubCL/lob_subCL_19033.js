/************************************
*@Description: seqDB-19033 主表挂载已有lob的子表
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
   var mainCLName = "mainCL_19033";
   var subCLName = "subCL_19033";
   var filePath = WORKDIR + "/lob19033/";
   var fileName = "file19033";
   var fileFullPath = filePath + fileName
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );
   var subCL = commCreateCL( db, csName, subCLName );
   var lobOids1 = insertLob( subCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190801" );
   var lobOids2 = insertLob( subCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190901" );

   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190831" } } );

   checkLobMD5( mainCL, lobOids1, fileMD5 );
   try
   {
      checkLobMD5( mainCL, lobOids2, fileMD5 );
      throw 0;
   }
   catch( e )
   {
      if( e !== -135 )
      {
         throw buildException( "get lob", e, "reads lobs that are not in the partition range: " + lobOids2, -135, e );
      }
   }

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
}
