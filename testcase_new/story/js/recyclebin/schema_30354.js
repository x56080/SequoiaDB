/******************************************************************************
 * @Description   : seqDB-30354:修改外部模式后强制恢复truncate
 * @Author        : liuli
 * @CreateTime    : 2023.03.07
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30354";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", ReadDefault: 10 } };
testConf.clName = COMMCLNAME + "_30354";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( args )
{
   var originName = COMMCSNAME + "." + testConf.clName;

   cleanRecycleBin( db, COMMCSNAME );
   var dbcl = args.testCL;
   var schema = args.testSchema;
   dbcl.addSchema( testConf.schemaName );

   var docs = [];
   var expResult = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
   }
   dbcl.insert( docs );

   // 集合执行truncate
   dbcl.truncate();

   // 外部模式增加字段，设置读默认值
   schema.addColumn( "c", { Type: "int32", ReadDefault: 20 } );

   var recycleName = getOneRecycleName( db, originName, "Truncate" );
   // 恢复truncate项目
   assert.tryThrow( SDB_RECYCLE_CONFLICT, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   // 校验外部模式被删除
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( testConf.schemaName );
   } );

   // 再次创建同名外部模式
   db.createSchema( testConf.schemaName, testConf.schemaDef );
   dbcl.addSchema( testConf.schemaName );

   // 校验外部模式和集合绑定关系
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );
}