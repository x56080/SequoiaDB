/******************************************************************************
 * @Description   : seqDB-34333:挂载同一数据源的多个子表后修改子表属性
 * @Author        : Suqiang Lin
 * @CreateTime    : 2025.10.22
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var dataSrcName = "datasrc34333b";
   var csName = "cs_34333b";
   var mainCLName = "main_34333";
   var clName1 = "cl_34333_1";
   var clName2 = "cl_34333_2";
   var clName3 = "cl_34333_3";
   var dsMainCLName1 = "main_34333_1";
   var dsMainCLName2 = "main_34333_2";
   var dsCSName = "datasrcCS_34333b";

   var filePath = WORKDIR + "/lob34333b/";
   var fileName = "filelob_34333b";
   var fileSize = 1024;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );

   var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd ); 
   commDropCS( datasrcDB, dsCSName );
   clearDataSource( csName, dataSrcName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   commCreateCL( datasrcDB, dsCSName, clName2 );
   commCreateCL( datasrcDB, dsCSName, clName3 );
   var mainCLOptions = { IsMainCL: true, ShardingKey: { date: 1 },
                         LobShardingKeyFormat: "YYYYMMDD" } ;
   commCreateCL( datasrcDB, dsCSName, dsMainCLName1, mainCLOptions );
   commCreateCL( datasrcDB, dsCSName, dsMainCLName2, mainCLOptions );

   var mainCL1 = datasrcDB.getCS( dsCSName ).getCL( dsMainCLName1 );
   mainCL1.attachCL( dsCSName + "." + clName2,
         { LowBound: { date: 19990102 }, UpBound: { date: 19990103 } } );
   mainCL1.attachCL( dsCSName + "." + clName3,
         { LowBound: { date: 19990103 }, UpBound: { date: 19990104 } } );

   commCreateCL( db, csName, mainCLName, mainCLOptions );
   commCreateCL( db, csName, clName1 );
   commCreateCL( db, csName, clName2,
         { DataSource: dataSrcName, Mapping: dsCSName + "." + clName2 } );
   commCreateCL( db, csName, clName3,
         { DataSource: dataSrcName, Mapping: dsCSName + "." + clName3 } );

   var mainCL = db.getCS( csName ).getCL( mainCLName );
   mainCL.attachCL( csName + "." + clName1,
         { LowBound: { date: 19990101 }, UpBound: { date: 19990102 } } );
   mainCL.attachCL( csName + "." + clName2,
         { LowBound: { date: 19990102 }, UpBound: { date: 19990103 } } );
   mainCL.attachCL( csName + "." + clName3,
         { LowBound: { date: 19990103 }, UpBound: { date: 19990104 } } );


   // 1. rename data source main cl
   datasrcDB.getCS( dsCSName ).renameCL(
         dsMainCLName1, dsMainCLName1 + "_ren" );
   var lobID = mainCL.createLobID( "1999-1-2-00.00.00.000000" );
   lobAndCheckResult ( mainCL, lobID, filePath, fileName, fileMD5 )
   datasrcDB.getCS( dsCSName ).renameCL(
         dsMainCLName1 + "_ren", dsMainCLName1 );

   // 2. detach from data source main cl
   mainCL1.detachCL( dsCSName + "." + clName3 );
   lobAndCheckResult ( mainCL, lobID, filePath, fileName, fileMD5 )

   // 3. attach to other data source main cl
   var mainCL2 = datasrcDB.getCS( dsCSName ).getCL( dsMainCLName2 );
   mainCL2.attachCL( dsCSName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );
   lobAndCheckResult ( mainCL, lobID, filePath, fileName, fileMD5 )

   // 4. data source attach bounds mismatch
   mainCL2.detachCL( dsCSName + "." + clName3 );
   mainCL1.attachCL( dsCSName + "." + clName3,
         { LowBound: { a: 4 }, UpBound: { a: 6 } } );
   lobAndCheckResult ( mainCL, lobID, filePath, fileName, fileMD5 )

   // restore
   mainCL1.detachCL( dsCSName + "." + clName3 );
   mainCL1.attachCL( dsCSName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );
   lobAndCheckResult ( mainCL, lobID, filePath, fileName, fileMD5 )

   commDropCS( datasrcDB, dsCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function lobAndCheckResult ( dbcl, lobID, filePath, fileName, fileMD5 )
{
   var lobName = "checkputlob34333b";
   //putLob 
   dbcl.putLob( filePath + fileName );

   //getLob  
   dbcl.getLob( lobID, filePath + lobName );
   var actMD5 = File.md5( filePath + lobName );
   assert.equal( fileMD5, actMD5 );

   //listLob
   var rc = dbcl.listLobs();
   var lobNum = 0;
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      lobNum++;
   }
   rc.close();
   assert.equal( 1, lobNum );

   // deleteLob
   dbcl.deleteLob( lobID );

   deleteTmpFile( filePath + lobName );
}

