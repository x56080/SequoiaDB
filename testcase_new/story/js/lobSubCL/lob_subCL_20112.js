/************************************
*@Description: seqDB-20112 使用不同连接进行挂载子表和插入lob
*@author:      luweikang
*@createDate:  2019.10.28
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
   var groups = commGetGroups( db, "", "", true, false, true );
   for( var i = 0; i < groups.length; i++ )
   {
      var coordGroup = groups[i];
      if( coordGroup[0].GroupName == "SYSCoord" )
      {
         break;
      }
   }
   if( coordGroup.length < 3 )
   {
      println( "--least two coord---" );
      return;
   }
   var coordA = coordGroup[1].HostName + ":" + coordGroup[1].svcname;
   var coordB = coordGroup[2].HostName + ":" + coordGroup[2].svcname;
   var dbA = new Sdb( coordA );
   var dbB = new Sdb( coordB );

   var csName = COMMCSNAME;
   var mainCLName = "mainCL_20112";
   var subCLName1 = "subCL_20112_1";
   var subCLName2 = "subCL_20112_2";
   var filePath = WORKDIR + "/lob20112/";
   var fileName = "file20112";
   var fileFullPath = filePath + fileName;
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName1 );
   commDropCL( db, csName, subCLName2 );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( dbA, csName, mainCLName, options, true, false, "create main cl" );
   commCreateCL( dbA, csName, subCLName1 );
   commCreateCL( dbA, csName, subCLName2 );

   dbB.getCS( csName ).getCL( mainCLName ).attachCL( csName + "." + subCLName1, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190805" } } );
   dbB.getCS( csName ).getCL( mainCLName ).attachCL( csName + "." + subCLName2, { "LowBound": { "date": "20190805" }, "UpBound": { "date": "20190810" } } );

   try
   {
      mainCL.attachCL( csName + "." + subCLName1, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190805" } } );
      throw "ERR_RELINK_CL";
   }
   catch( e )
   {
      if( e != -235 )
      {
         throw new Error( "attach error, exp: -235, act: " + e );
      }
   }

   try
   {
      mainCL.attachCL( csName + "." + subCLName2, { "LowBound": { "date": "20190805" }, "UpBound": { "date": "20190810" } } );
      throw "ERR_RELINK_CL";
   }
   catch( e )
   {
      if( e != -235 )
      {
         throw new Error( "attach error, exp: -235, act: " + e );
      }
   }

   var lobOids = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 2, "20190801" );
   checkLobMD5( mainCL, lobOids, fileMD5 );

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
}

