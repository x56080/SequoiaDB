/******************************************************************************
 * @Description   : seqDB-30355:集合绑定外部模式，恢复dropCL项目
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30355";
testConf.schemaDef = { "a": { Type: "int32", ReadDefault: 10, WriteDefault: 20 }, "b": { Type: "int32", ReadDefault: 30, WriteDefault: 40 } };
testConf.clName = COMMCLNAME + "_30355";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( args )
{
   var originName = COMMCSNAME + "." + testConf.clName;
   var schemaName2 = "schema_30355_2";
   var schemaName3 = "schema_30355_3";
   commDropSchema( db, schemaName2 );
   commDropSchema( db, schemaName3 );

   cleanRecycleBin( db, COMMCSNAME );
   var dbcl = args.testCL;

   // 绑定外部模式
   dbcl.addSchema( testConf.schemaName );

   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 40 } );
      expPrimalResult.push( { a: i, b: 40 } );
   }
   dbcl.insert( docs );

   // 删除集合
   var dbcs = db.getCS( COMMCSNAME );
   dbcs.dropCL( testConf.clName );

   var recycleName = getOneRecycleName( db, originName, 'Drop' );
   db.getRecycleBin().returnItem( recycleName );

   // 校验不存在外部模式
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( testConf.schemaName );
   } )

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   var schemaDef = {
      "a": { Type: "int32", ReadDefault: 100, WriteDefault: 200 },
      "b": { Type: "int32", ReadDefault: 300, WriteDefault: 400 }
   };
   commCreateSchema( db, schemaName2, schemaDef );
   dbcl.addSchema( schemaName2 );
   println( dbcl.getInternalSchema() );
   checkAddSchema( db, csName, clName, schemaName )

   // commCreateSchema( db, schemaName3, testConf.schemaDef );
   // dbcl.addSchema( schemaName3 );

   // 校验内部模式
   var expInternalColumnDef = {
      "a": {
         "ReadDefault": 10,
         "WriteDefault": 200
      },
      "b": {
         "ReadDefault": 30,
         "WriteDefault": 400
      }
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   // 校验外部模式和集合绑定关系
   checkAddSchema( db, COMMCSNAME, testConf.clName, schemaName2 );
   // checkAddSchema( db, COMMCSNAME, testConf.clName, schemaName3 );
   commDropCL( db, COMMCSNAME, testConf.clName );
}