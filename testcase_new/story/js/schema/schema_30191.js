/******************************************************************************
 * @Description   : seqDB-30191：主表绑定外部模式删除子表
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.02.27
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30191";
testConf.schemaDef = { "a": { Type: "int32" } };
main( test );

function test ()
{
   var mainCLName = "mainCL_30191";
   var subCLName1 = "subCL_30191_1";
   var subCLName2 = "subCL_30191_2";
   var schemaName = "schemaName_30191";
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropSchema( db, schemaName );

   // 创建主表绑定外部模式
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   maincl.addSchema( testConf.schemaName );

   // 挂载子表
   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   // 插入数据
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 120; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   maincl.insert( docs );

   // 删除子表
   maincl.detachCL( COMMCSNAME + "." + subCLName2 );

   // 检查主表绑定外部模式
   checkAddSchema( db, COMMCSNAME, mainCLName, testConf.schemaName );

   // 再次插入数据
   docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   maincl.insert( docs );
   expResult = expResult.concat( docs );
   expResult = expResult.concat( docs );

   // 校验数据
   expResult.sort( sortBy( 'a' ) );
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = maincl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   commDropCL( db, COMMCSNAME, mainCLName );
}

