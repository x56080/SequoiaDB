/******************************************************************************
 * @Description   : seqDB-30356:集合绑定外部模式，重命名恢复dropCL项目
 * @Author        : liuli
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30356";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10 } };
testConf.clName = COMMCLNAME + "_30356";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( args )
{
   var clName = "cl_30356";
   var schemaName = "schema_30356";
   var originName = COMMCSNAME + "." + testConf.clName;

   cleanRecycleBin( db, COMMCSNAME );
   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );
   var dbcs = args.testCS;
   var dbcl = args.testCL;
   dbcl.addSchema( testConf.schemaName );

   var docs = [];
   var expResult = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
   }
   dbcl.insert( docs );

   // 删除集合
   dbcs.dropCL( testConf.clName );

   // 重命名恢复dropCL项目
   var recycleName = getOneRecycleName( db, originName, "Drop" );
   db.getRecycleBin().returnItemToName( recycleName, COMMCSNAME + "." + clName );

   // 校验外部模式被删除
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( testConf.schemaName );
   } );

   // 再次创建外部模式，名称相同，定义不同
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10, WriteDefault: 20 } };
   db.createSchema( testConf.schemaName, schemaDef );

   // 集合绑定外部模式
   var dbcl = dbcs.getCL( clName );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.addSchema( testConf.schemaName );
   } );

   // 再次创建外部模式，名称不同，定义相同
   db.createSchema( schemaName, testConf.schemaDef );

   dbcl.addSchema( schemaName );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );
}