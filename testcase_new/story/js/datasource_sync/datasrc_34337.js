/******************************************************************************
 * @Description   : seqDB-34337:挂载同一数据源的多个子表后进行命令操作
 * @Author        : Suqiang Lin
 * @CreateTime    : 2025.10.22
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var dataSrcName = "datasrc34337";
   var csName = "cs_34337";
   var mainCLName = "main_34337";
   var clName1 = "cl_34337_1";
   var clName2 = "cl_34337_2";
   var clName3 = "cl_34337_3";
   var dsMainCLName1 = "main_34337_1";

   commDropCS( datasrcDB, csName );
   clearDataSource( csName, dataSrcName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   commCreateCL( datasrcDB, csName, clName2 );
   commCreateCL( datasrcDB, csName, clName3 );
   commCreateCL( datasrcDB, csName, dsMainCLName1,
         { IsMainCL: true, ShardingKey: { a: 1 } } );

   var mainCL1 = datasrcDB.getCS( csName ).getCL( dsMainCLName1 );
   mainCL1.attachCL( csName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );
   mainCL1.attachCL( csName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );

   commCreateCL( db, csName, mainCLName,
         { IsMainCL: true, ShardingKey: { a: 1 } } );
   commCreateCL( db, csName, clName1 );
   commCreateCL( db, csName, clName2, { DataSource: dataSrcName } );
   commCreateCL( db, csName, clName3, { DataSource: dataSrcName } );

   var mainCL = db.getCS( csName ).getCL( mainCLName );
   mainCL.attachCL( csName + "." + clName1,
         { LowBound: { a: 1 }, UpBound: { a: 2 } } );
   mainCL.attachCL( csName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );
   mainCL.attachCL( csName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );

   mainCL.insert({ a: 1 });
   mainCL.insert({ a: 2 });
   mainCL.insert({ a: 3 });

   result = mainCL.count();
   assert.equal( result, 3 );
   result = mainCL.count({ a: { $gte: 2 } });
   assert.equal( result, 2 );
   result = mainCL.count({ a: { $gte: 3 } });
   assert.equal( result, 1 );

   // Note: test listLobs command in seqDB-34336

   // jsonFormat(false);
   result = mainCL.getDetail();
   var numReturned = 0;
   var totalRecords = 0;
   var hasDSGroup = false;
   while ( record = result.next() )
   {
      var obj = record.toObj();
      println( JSON.stringify( obj ) );
      if ( obj.Details[0].GroupName == "$null" )
      {
         hasDSGroup = true;
      }
      totalRecords += obj.Details[0].TotalRecords;
      numReturned += 1;
   }
   assert.equal( numReturned, 2 );
   assert.equal( hasDSGroup, true );

   result = mainCL.getDetail(true);
   var numReturned = 0;
   var totalRecords = 0;
   while ( record = result.next() )
   {
      var obj = record.toObj();
      println( JSON.stringify( obj ) );
      totalRecords += obj.Details[0].TotalRecords;
      numReturned += 1;
   }
   assert.equal( totalRecords, 3 );
   assert.equal( numReturned, 1 );

   mainCL.truncate();
   result = mainCL.count();
   assert.equal( result, 0 );

   commDropCS( datasrcDB, csName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}
