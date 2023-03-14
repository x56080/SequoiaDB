/******************************************************************************
 * @Description   : seqDB-30349:绑定外部模式的主表detachCL，外部模式存在默认值
 * @Author        : liuli
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.06
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var mainCLName = "mainCL_30349";
   var subCLName1 = "subCL_30349_1";
   var subCLName2 = "subCL_30349_2";
   var schemaName = "schema_30349";

   // 建表绑定开启内部模式绑定外部模式
   testSchema(
      function()
      {
         var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
         commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
         commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
         maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
         maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
         maincl.addSchema( schemaName );
         return maincl;
      },
      function()
      { }, mainCLName, schemaName, subCLName1 );

   // 建表不开启内部模式，插入数据后开启内部模式，绑定外部模式
   testSchema(
      function()
      {
         var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
         commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
         commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
         maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
         maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
         return maincl;
      },
      function( maincl )
      {
         maincl.alter( { EnableInfoSchema: true } );
         maincl.addSchema( schemaName );
      }, mainCLName, schemaName, subCLName1 );
}

function testSchema ( func1, func2, mainCLName, schemaName, subCLName1 )
{
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropSchema( db, schemaName );

   var schemaDef = { "b": { Type: "int32", WriteDefault: 20 }, "c": { Type: "int32", ReadDefault: 10 } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var maincl = func1();

   // 插入数据，均为外部模式中存在的字段
   var expResult1 = [];
   var expPrimalResult1 = [];
   var expResult2 = [];
   var expPrimalResult2 = [];
   var expMainResult2 = [];
   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { a: i, b: i } );
      if( i < 1000 )
      {
         expResult1.push( { a: i, b: i, c: 10 } );
         expPrimalResult1.push( { a: i, b: i } );
      }
      else
      {
         expResult2.push( { a: i, b: i, c: 10 } );
         expPrimalResult2.push( { a: i, b: i } );
      }
      expMainResult2.push( { a: i, b: i, c: 10, d: 20 } );
   }
   maincl.insert( docs );

   func2( maincl );

   // detachCL一个子表
   maincl.detachCL( COMMCSNAME + "." + subCLName1 );

   // 直连子表校验数据
   var subcl = db.getCS( COMMCSNAME ).getCL( subCLName1 );
   var actResult = subcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult1 );
   var actResult = subcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult1 );

   // 主表校验数据
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult2 );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult2 );

   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   // 重新挂载主表后校验数据
   var expMainResult = [];
   var expMainPrimalResult = [];
   expMainResult.concat( expResult1, expResult2 );
   expMainPrimalResult.concat( expPrimalResult1, expPrimalResult2 );
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expMainResult );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expMainPrimalResult );

   // 外部模式新增字段，添加读默认值，再次校验数据
   schema.addColumn( "d", { Type: "int32", ReadDefault: 20 } );
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expMainResult2 );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expMainPrimalResult );

   commDropCL( db, COMMCSNAME, mainCLName );
}