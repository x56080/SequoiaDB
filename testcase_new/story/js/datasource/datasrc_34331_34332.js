/******************************************************************************
 * @Description   : seqDB-34331:挂载同一数据源的多个不同属性的子表
 * @Description   : seqDB-34332:挂载同一数据源的多个子表期间修改子表属性
 * @Author        : Suqiang Lin
 * @CreateTime    : 2025.10.22
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipTest = false;

main( test );

function test ()
{
   var dataSrcName = "datasrc34331";
   var csName = "cs_34331";
   var mainCLName = "main_34331";
   var clName1 = "cl_34331_1";
   var clName2 = "cl_34331_2";
   var clName3 = "cl_34331_3";
   var dsMainCLName1 = "main_34331_1";
   var dsMainCLName2 = "main_34331_2";
   var dsCSName = "datasrcCS_34331";

   var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd ); 
   commDropCS( datasrcDB, dsCSName );
   clearDataSource( csName, dataSrcName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   commCreateCL( datasrcDB, dsCSName, clName2 );
   commCreateCL( datasrcDB, dsCSName, clName3 );
   commCreateCL( datasrcDB, dsCSName, dsMainCLName1,
         { IsMainCL: true, ShardingKey: { a: 1 } } );
   commCreateCL( datasrcDB, dsCSName, dsMainCLName2,
         { IsMainCL: true, ShardingKey: { a: 1 } } );

   commCreateCL( db, csName, mainCLName,
         { IsMainCL: true, ShardingKey: { a: 1 } } );
   commCreateCL( db, csName, clName1 );
   commCreateCL( db, csName, clName2,
         { DataSource: dataSrcName, Mapping: dsCSName + "." + clName2 } );
   commCreateCL( db, csName, clName3,
         { DataSource: dataSrcName, Mapping: dsCSName + "." + clName3 } );

   // 1. without data source main cl
   var mainCL = db.getCS( csName ).getCL( mainCLName );
   mainCL.attachCL( csName + "." + clName1,
         { LowBound: { a: 1 }, UpBound: { a: 2 } } );
   mainCL.attachCL( csName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );
   assert.tryThrow( [SDB_OPTION_NOT_SUPPORT], function() 
   {
      mainCL.attachCL( csName + "." + clName3,
            { LowBound: { a: 3 }, UpBound: { a: 4 } } );
   } );

   // 2. different data source main cl
   var mainCL1 = datasrcDB.getCS( dsCSName ).getCL( dsMainCLName1 );
   mainCL1.attachCL( dsCSName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );
   var cl2 = db.getCS( csName ).getCL( clName2 );
   cl2.alter( { DSMainCLName: dsCSName + "." + dsMainCLName1 } );

   var mainCL2 = datasrcDB.getCS( dsCSName ).getCL( dsMainCLName2 );
   mainCL2.attachCL( dsCSName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );
   var cl3 = db.getCS( csName ).getCL( clName3 );
   cl3.alter( { DSMainCLName: dsCSName + "." + dsMainCLName2 } );

   assert.tryThrow( [SDB_OPTION_NOT_SUPPORT], function() 
   {
      mainCL.attachCL( csName + "." + clName3,
            { LowBound: { a: 3 }, UpBound: { a: 4 } } );
   } );

   mainCL1.detachCL( dsCSName + "." + clName2 );
   cl2.alter( { DSMainCLName: "" } );
   assert.tryThrow( [SDB_OPTION_NOT_SUPPORT], function() 
   {
      mainCL.attachCL( csName + "." + clName3,
            { LowBound: { a: 3 }, UpBound: { a: 4 } } );
   } );

   // 3. the same data source main cl
   mainCL2.attachCL( dsCSName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );
   cl2.alter( { DSMainCLName: dsCSName + "." + dsMainCLName2 } );
   mainCL.attachCL( csName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );

   // 4. detach on data source before attach
   mainCL.detachCL( csName + "." + clName2 );
   mainCL2.detachCL( dsCSName + "." + clName2 ) ;
   cl2.alter( { DSMainCLName: "" } );
   assert.tryThrow( [SDB_OPTION_NOT_SUPPORT], function() 
   {
      mainCL.attachCL( csName + "." + clName2,
            { LowBound: { a: 2 }, UpBound: { a: 3 } } );
   } );

   // 5. change main cl before attach
   mainCL2.detachCL( dsCSName + "." + clName3 );
   cl3.alter( { DSMainCLName: "" } );
   mainCL1.attachCL( dsCSName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );
   cl3.alter( { DSMainCLName: dsCSName + "." + dsMainCLName1 } );
   mainCL1.attachCL( dsCSName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );
   cl2.alter( { DSMainCLName: dsCSName + "." + dsMainCLName1 } );
   mainCL.attachCL( csName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );

   // 6. rename main cl before attach
   mainCL.detachCL( csName + "." + clName2 );
   datasrcDB.getCS( dsCSName ).renameCL(
         dsMainCLName1, dsMainCLName1 + "_ren"  );
   cl2.alter( { DSMainCLName: dsCSName + "." + dsMainCLName1 + "_ren" } );
   cl3.alter( { DSMainCLName: dsCSName + "." + dsMainCLName1 + "_ren" } );
   mainCL.attachCL( csName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );

   mainCL.insert([ { a: 1 }, { a: 2 }, { a: 3 } ]);
   assert.equal( mainCL.count(), 3 );

   commDropCS( datasrcDB, dsCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}
