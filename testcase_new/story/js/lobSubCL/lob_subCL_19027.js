/************************************
*@Description:  seqDB-19027 删除主表集合空间，主表已挂载子表
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
   var mainCSName = "mainCS_19027";
   var mainCLName = "mainCL_19027";
   var subCSName = "subCS_19027";
   var subCLName = "subCL_19027";
   var filePath = WORKDIR + "/lob19027/";
   var fileName = "file19027";
   var fileFullPath = filePath + fileName;
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCS( db, mainCSName );
   commDropCS( db, subCSName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCLByOption( db, mainCSName, mainCLName, options, true, false, "create main cl" );
   commCreateCL( db, subCSName, subCLName );

   mainCL.attachCL( subCSName + "." + subCLName, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190831" } } );
   var lobOids = insertLob( mainCL, fileFullPath, "YYYYMMDD", 0, 10, 1, "20190808" );


   db.dropCS( mainCSName );

   try
   {
      db.getCS( mainCSName );
      throw 0;
   }
   catch( e )
   {
      if( e !== -34 )
      {
         throw buildException( "check mainCL cs", e, "drop mainCL cs: " + mainCSName, -34, e );
      }
   }

   var subCL = db.getCS( subCSName ).getCL( subCLName );
   checkLobMD5( subCL, lobOids, fileMD5 );

   deleteTmpFile( filePath );
   commDropCS( db, mainCSName );
   commDropCS( db, subCSName );
}

