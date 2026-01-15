/******************************************************************************
 * @Description   : seqDB-34333:挂载同一数据源的多个子表后修改子表属性
 * @Author        : Suqiang Lin
 * @CreateTime    : 2025.10.22
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var dataSrcName = "datasrc34333";
   var csName = "cs_34333";
   var mainCLName = "main_34333";
   var clName1 = "cl_34333_1";
   var clName2 = "cl_34333_2";
   var clName3 = "cl_34333_3";
   var dsMainCLName1 = "main_34333_1";
   var dsMainCLName2 = "main_34333_2";
   var dsCSName = "datasrcCS_34333";

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

   var mainCL1 = datasrcDB.getCS( dsCSName ).getCL( dsMainCLName1 );
   mainCL1.attachCL( dsCSName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );
   mainCL1.attachCL( dsCSName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );

   commCreateCL( db, csName, mainCLName,
         { IsMainCL: true, ShardingKey: { a: 1 } } );
   commCreateCL( db, csName, clName1 );
   commCreateCL( db, csName, clName2,
         { DataSource: dataSrcName, Mapping: dsCSName + "." + clName2 } );
   commCreateCL( db, csName, clName3,
         { DataSource: dataSrcName, Mapping: dsCSName + "." + clName3 } );

   var mainCL = db.getCS( csName ).getCL( mainCLName );
   mainCL.attachCL( csName + "." + clName1,
         { LowBound: { a: 1 }, UpBound: { a: 2 } } );
   mainCL.attachCL( csName + "." + clName2,
         { LowBound: { a: 2 }, UpBound: { a: 3 } } );
   mainCL.attachCL( csName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );

   // 1. rename data source main cl
   datasrcDB.getCS( dsCSName ).renameCL(
         dsMainCLName1, dsMainCLName1 + "_ren" );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.insert([ { a: 1 }, { a: 2 }, { a: 3 } ]);
   } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.update({ $set: { b: 1 } }, { a: { $gt: 1 } });
   } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      println( mainCL.find({ a: { $gt: 1 } }) );
   } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.remove({ a: { $gt: 1 } });
   } );
   result = mainCL.insert([ { a: 2 } ]);
   assert.equal( result.toString(),
         '{ "InsertedNum": 1, "DuplicatedNum": 0, "ModifiedNum": 0 }');
   result = mainCL.update({ $set: { b: 1 } }, { a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }');
   result = mainCL.find({ a: { $et: 2 } });
   assert.equal( result.next().toString(),
         '{ "a": 2, "b": 1 }');
   result = mainCL.remove({ a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "DeletedNum": 1 }');
   datasrcDB.getCS( dsCSName ).renameCL(
         dsMainCLName1 + "_ren", dsMainCLName1 );

   // 2. detach from data source main cl
   mainCL1.detachCL( dsCSName + "." + clName3 );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.insert([ { a: 1 }, { a: 2 }, { a: 3 } ]);
   } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.update({ $set: { b: 1 } }, { a: { $gt: 1 } });
   } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      println( mainCL.find({ a: { $gt: 1 } }) );
   } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.remove({ a: { $gt: 1 } });
   } );
   result = mainCL.insert([ { a: 2 } ]);
   assert.equal( result.toString(),
         '{ "InsertedNum": 1, "DuplicatedNum": 0, "ModifiedNum": 0 }');
   result = mainCL.update({ $set: { b: 1 } }, { a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }');
   result = mainCL.find({ a: { $et: 2 } });
   assert.equal( result.next().toString(),
         '{ "a": 2, "b": 1 }');
   result = mainCL.remove({ a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "DeletedNum": 1 }');

   // 3. attach to other data source main cl
   var mainCL2 = datasrcDB.getCS( dsCSName ).getCL( dsMainCLName2 );
   mainCL2.attachCL( dsCSName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.insert([ { a: 1 }, { a: 2 }, { a: 3 } ]);
   } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.update({ $set: { b: 1 } }, { a: { $gt: 1 } });
   } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      println( mainCL.find({ a: { $gt: 1 } }) );
   } );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.remove({ a: { $gt: 1 } });
   } );
   result = mainCL.insert([ { a: 2 } ]);
   assert.equal( result.toString(),
         '{ "InsertedNum": 1, "DuplicatedNum": 0, "ModifiedNum": 0 }');
   result = mainCL.update({ $set: { b: 1 } }, { a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }');
   result = mainCL.find({ a: { $et: 2 } });
   assert.equal( result.next().toString(),
         '{ "a": 2, "b": 1 }');
   result = mainCL.remove({ a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "DeletedNum": 1 }');

   // 4. data source attach bounds mismatch
   mainCL2.detachCL( dsCSName + "." + clName3 );
   mainCL1.attachCL( dsCSName + "." + clName3,
         { LowBound: { a: 4 }, UpBound: { a: 6 } } );
   assert.tryThrow( [SDB_BOUND_INVALID], function() 
   {
      mainCL.insert([ { a: 1 }, { a: 2 }, { a: 3 } ]);
   } );
   assert.tryThrow( [SDB_BOUND_INVALID], function() 
   {
      mainCL.update({ $set: { b: 1 } }, { a: { $gt: 1 } });
   } );
   assert.tryThrow( [SDB_BOUND_INVALID], function() 
   {
      println( mainCL.find({ a: { $gt: 1 } }) );
   } );
   assert.tryThrow( [SDB_BOUND_INVALID], function() 
   {
      mainCL.remove({ a: { $gt: 1 } });
   } );
   result = mainCL.insert([ { a: 2 } ]);
   assert.equal( result.toString(),
         '{ "InsertedNum": 1, "DuplicatedNum": 0, "ModifiedNum": 0 }');
   result = mainCL.update({ $set: { b: 1 } }, { a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }');
   result = mainCL.find({ a: { $et: 2 } });
   assert.equal( result.next().toString(),
         '{ "a": 2, "b": 1 }');
   result = mainCL.remove({ a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "DeletedNum": 1 }');

   // restore
   mainCL1.detachCL( dsCSName + "." + clName3 );
   mainCL1.attachCL( dsCSName + "." + clName3,
         { LowBound: { a: 3 }, UpBound: { a: 4 } } );
   mainCL.insert([ { a: 1 }, { a: 2 }, { a: 3 } ]);
   mainCL.update({ $set: { b: 1 } }, { a: { $gt: 1 } });
   println( mainCL.find({ a: { $gt: 1 } }) );
   mainCL.remove({ a: { $gt: 1 } });

   result = mainCL.insert([ { a: 2 } ]);
   assert.equal( result.toString(),
         '{ "InsertedNum": 1, "DuplicatedNum": 0, "ModifiedNum": 0 }');
   result = mainCL.update({ $set: { b: 1 } }, { a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }');
   result = mainCL.find({ a: { $et: 2 } });
   assert.equal( result.next().toString(),
         '{ "a": 2, "b": 1 }');
   result = mainCL.remove({ a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "DeletedNum": 1 }');

   commDropCS( datasrcDB, dsCSName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}
