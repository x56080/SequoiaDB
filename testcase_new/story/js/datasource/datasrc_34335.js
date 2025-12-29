/******************************************************************************
 * @Description   : seqDB-34335:挂载同一数据源的多个子表后进行增删改查
 * @Author        : Suqiang Lin
 * @CreateTime    : 2025.10.22
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var dataSrcName = "datasrc34335";
   var csName = "cs_34335";
   var mainCLName = "main_34335";
   var clName1 = "cl_34335_1";
   var clName2 = "cl_34335_2";
   var clName3 = "cl_34335_3";
   var dsMainCLName1 = "main_34335_1";

   var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd ); 
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
   datasrcDB.getCS( csName ).getCL( clName2 ).
         createIndex( "uniq", { a: 1 }, true, true );
   datasrcDB.getCS( csName ).getCL( clName3 ).
         createIndex( "uniq", { a: 1 }, true, true );
   mainCL.createIndex( "uniq", { a: 1 }, true, true );

   // match single sub collection
   mainCL.insert({ a: 1 });
   mainCL.insert({ a: 2 });
   mainCL.insert({ a: 3 });
   result = mainCL.update({ $set: { b: 1 } }, { a: { $et: 1 } });
   jsonFormat(false);
   assert.equal( result.toString(),
         '{ "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }');
   result = mainCL.insert({ a: 1, b: 1, c: 1 },
         { UpdateOnDup: true, Update: { $inc: { b: {
            Value: 1,
            Min: -2147483648,
            Max: 2147483647,
            Default: null
         } } } } );
   assert.equal( result.toString(),
         '{ "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 1 }');
   result = mainCL.update({ $set: { b: 1 } }, { a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }');
   result = mainCL.insert({ a: 2, b: 2, c: 2 },
         { UpdateOnDup: true, Update: { $inc: { b: {
            Value: 1,
            Min: -2147483648,
            Max: 2147483647,
            Default: null
         } } } } );
   assert.equal( result.toString(),
         '{ "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 1 }');
   result = mainCL.update({ $set: { b: 1 } }, { a: { $et: 3 } });
   assert.equal( result.toString(),
         '{ "UpdatedNum": 1, "ModifiedNum": 1, "InsertedNum": 0 }');
   result = mainCL.insert({ a: 3, b: 3, c: 3 },
         { UpdateOnDup: true, Update: { $inc: { b: {
            Value: 1,
            Min: -2147483648,
            Max: 2147483647,
            Default: null
         } } } } );
   assert.equal( result.toString(),
         '{ "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 1 }');
   result = mainCL.find({ a: { $et: 1 } }, { _id: { $include: 0 } }).sort({ _id: 1 });
   assert.equal( result.next().toString(),
         '{ "a": 1, "b": 2 }');
   result = mainCL.find({ a: { $et: 2 } }, { _id: { $include: 0 } }).sort({ _id: 1 });
   assert.equal( result.next().toString(),
         '{ "a": 2, "b": 2 }');
   result = mainCL.find({ a: { $et: 3 } }, { _id: { $include: 0 } }).sort({ _id: 1 });
   assert.equal( result.next().toString(),
         '{ "a": 3, "b": 2 }');
   result = mainCL.remove({ a: { $et: 1 } });
   assert.equal( result.toString(),
         '{ "DeletedNum": 1 }');
   result = mainCL.remove({ a: { $et: 2 } });
   assert.equal( result.toString(),
         '{ "DeletedNum": 1 }');
   result = mainCL.remove({ a: { $et: 3 } });
   assert.equal( result.toString(),
         '{ "DeletedNum": 1 }');

   // match multiple sub collections
   result = mainCL.insert([ { a: 1 }, { a: 2 }, { a: 3 } ]);
   assert.equal( result.toString(),
         '{ "InsertedNum": 3, "DuplicatedNum": 0, "ModifiedNum": 0 }');
   result = mainCL.update({ $set: { b: 1 } }, { a: { $gt: 1 } });
   assert.equal( result.toString(),
         '{ "UpdatedNum": 2, "ModifiedNum": 2, "InsertedNum": 0 }');
   result = mainCL.insert([{ a: 2, b: 2, c: 2 }, { a: 3, b: 3, c: 3 }],
         { UpdateOnDup: true, Update: { $inc: { b: {
            Value: 1,
            Min: -2147483648,
            Max: 2147483647,
            Default: null
         } } } } );
   assert.equal( result.toString(),
         '{ "InsertedNum": 0, "DuplicatedNum": 2, "ModifiedNum": 2 }');
   result = mainCL.find({ a: { $gt: 1 } }, { _id: { $include: 0 } }).sort({ _id: 1 });
   assert.equal( result.next().toString(),
         '{ "a": 2, "b": 2 }');
   assert.equal( result.next().toString(),
         '{ "a": 3, "b": 2 }');
   result = mainCL.remove({ a: { $gt: 1 } });
   assert.equal( result.toString(),
         '{ "DeletedNum": 2 }');

   commDropCS( datasrcDB, csName );
   commDropCS( db, csName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}
