/******************************************************************************
 * @Description   : seqDB-30200：集合中非贴源数据更新为贴源数据执行split
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.02.27
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30200";
testConf.schemaDef = { "a": { Type: "int32" } };
testConf.clName = COMMCLNAME + "_30200";
testConf.clOpt = { ShardingKey: { a: 1 }, "ShardingType": "hash", EnableInfoSchema: true };
testConf.useDstGroup = true;
testConf.useSrcGroup = true;
main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var schema = testPara.testSchema;

   // 绑定外部模式
   dbcl.addSchema( testConf.schemaName );
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // 插入数据，为外部模式包含字段
   var doc = [];
   var expResult = [];
   var primalResult = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
   }
   primalResult = doc;
   dbcl.insert( doc );

   // 外部模式新增字段设置读默认值
   schema.addColumn( "b", { Type: "int32", ReadDefault: 10 } );

   // 更新非贴源字段，使字段变为贴源字段
   var cond = { a: 0 };
   dbcl.upsert( { $set: { b: 100 } }, cond );
   expResult.splice( 0, 1, { a: 0, b: 100 } );
   primalResult.splice( 0, 1, { a: 0, b: 100 } );

   // 切分部分数据
   dbcl.split( testPara.srcGroupName, testPara.dstGroupNames[0], 30 );

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );

   commDropCL( db, COMMCSNAME, testConf.clName );
}
