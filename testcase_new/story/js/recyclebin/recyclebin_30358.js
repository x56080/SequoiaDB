/******************************************************************************
 * @Description   : seqDB-30358:旧版本集合dropCL，新建同名集合，强制恢复dropCL项目
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30358";
testConf.schemaDef = { "a": { Type: "int32" } };
testConf.clName = COMMCLNAME + "_30358";

main( test );
function test ( args )
{
   cleanRecycleBin( db, COMMCSNAME );
   var schemaName = "schema_30358";
   commDropSchema( db, schemaName );
   var dbcl = args.testCL;
   var clName = testConf.clName;
   var originName = COMMCSNAME + "." + testConf.clName;

   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i } );
      expPrimalResult.push( { a: i } );
   }
   dbcl.insert( docs );

   // 删除集合
   var dbcs = db.getCS( COMMCSNAME );
   dbcs.dropCL( clName );

   // 创建同名集合，开启内部模式
   var dbcl1 = commCreateCL( db, COMMCSNAME, clName, { EnableInfoSchema: true } );

   // 绑定外部模式
   dbcl1.addSchema( testConf.schemaName );

   // 强行恢复dropCL项目
   var recycleName = getOneRecycleName( db, originName, "Drop" );
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   // 校验集合未开启内部模式
   assert.tryThrow( SDB_INTERNAL_SCHEMA_NOT_ENABLED, function()
   {
      dbcl.getInternalSchema();
   } )

   // 外部模式被删除
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( testConf.schemaName );
   } )

   dbcl.alter( { EnableInfoSchema: true } );
   var schemaDef = {
      "b": { Type: "int32", ReadDefault: 300, WriteDefault: 400 }
   };
   commCreateSchema( db, schemaName, schemaDef );

   dbcl.addSchema( schemaName );

   var expInternalColumnDef = {
      "b": {
         "ReadDefault": 300,
         "WriteDefault": 400
      }
   };
   checkInternalSchema( dbcl, expInternalColumnDef );
}