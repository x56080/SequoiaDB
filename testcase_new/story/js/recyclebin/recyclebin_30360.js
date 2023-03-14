/******************************************************************************
 * @Description   : seqDB-30360:存在绑定外部模式的集合，重命名恢复dropCS项目
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.07
 * @LastEditTime  : 2023.03.08
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( args )
{
   var newName = "newName_30360";
   var clName = "cl_30360";
   var csName = "cs_30360";
   cleanRecycleBin( db, newName );
   cleanRecycleBin( db, csName );
   commDropCS( db, csName );
   commDropCS( db, newName );

   var dbcl = commCreateCL( db, csName, clName, { EnableInfoSchema: true } );

   var schemaName1 = "schema_30360_1";
   var schemaName2 = "schema_30360_2";
   var schemaName3 = "schema_30360_3";
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );
   commDropSchema( db, schemaName3 );

   var schemaDef = {
      "a": { Type: "int32", ReadDefault: 10, WriteDefault: 20 },
      "b": { Type: "int32", ReadDefault: 30, WriteDefault: 40 }
   };
   commCreateSchema( db, schemaName1, schemaDef );

   dbcl.addSchema( schemaName1 );


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

   // 删除CS,重命名恢复dropCS项目
   db.dropCS( csName );
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItemToName( recycleName, newName );

   // 贴源、非贴源校验数据
   var dbcl = db.getCS( newName ).getCL( clName );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   var expInternalColumnDef = {
      "a": {
         "ReadDefault": 10,
         "WriteDefault": 20
      },
      "b": {
         "ReadDefault": 30,
         "WriteDefault": 40
      }
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   // 外部模式被删除
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      db.getSchema( schemaName1 );
   } )

   var schemaDef2 = {
      "a": { Type: "int32", ReadDefault: 100, WriteDefault: 200 },
      "b": { Type: "int32", ReadDefault: 300, WriteDefault: 400 }
   };
   commCreateSchema( db, schemaName2, schemaDef2 );
   dbcl.addSchema( schemaName2 );

   // commCreateSchema( db, schemaName3, schemaDef );
   // dbcl.addSchema( schemaName3 );

   // 校验外部模式和集合绑定关系
   checkAddSchema( db, newName, clName, schemaName2 );

   commDropCS( db, csName );
   commDropCS( db, newName );
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );
   commDropSchema( db, schemaName3 );
   cleanRecycleBin( db, newName );
   cleanRecycleBin( db, csName );
}