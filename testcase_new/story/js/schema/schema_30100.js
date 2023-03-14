/******************************************************************************
 * @Description   : seqDB-30100:绑定外部模式的主表挂载/卸载子表
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "mainCL_30100";
   var subCLName1 = "subCL_30100_1";
   var subCLName2 = "subCL_30100_2";
   var schemaName = "schemaName_30100";
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropSchema( db, schemaName );

   // 创建外部模式
   var schemaDef = { "a": { Type: "int32" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   maincl.addSchema( schemaName );

   // 插入数据使用外部模式字段
   var doc = [];
   var expResult1 = [];
   var expResult2 = [];
   var primalResult = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: i } );
      expResult1.push( { a: i } );
      expResult2.push( { a: i, b: 10 } );
      primalResult.push( { a: i } );
   }
   maincl.insert( doc );

   // 校验数据
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult1 );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   // 外部模式增加字段
   var columnDef = { Type: "int32", ReadDefault: 10 };
   schema.addColumn( "b", columnDef );

   // 卸载一个子表
   maincl.detachCL( COMMCSNAME + "." + subCLName2 );

   // 校验数据
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult2 );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   commDropCL( db, COMMCSNAME, mainCLName );
}

