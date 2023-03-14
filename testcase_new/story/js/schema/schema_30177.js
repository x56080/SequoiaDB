/******************************************************************************
 * @Description   : seqDB-30177:更新数据匹配非贴源字段
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30177";
testConf.schemaDef = { "a": { Type: "int32" } };
testConf.clName = COMMCLNAME + "_30177";
testConf.clOpt = { EnableInfoSchema: true, ReplSize: 0 }

main( test );
function test ( testPara )
{
   var schema = db.getSchema( testConf.schemaName );
   var dbcl = testPara.testCL;

   // 检查外部模式shema
   checkColumnDef( db, testConf.schemaName, testConf.schemaDef );

   // 集合绑定外部模式
   dbcl.addSchema( testConf.schemaName );

   // 写入数据，包含外部模式所有字段
   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 50; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: 30, b: 1, c: 2 } );
      expPrimalResult.push( { a: 30, b: 1, c: 2 } );
   }
   dbcl.insert( docs );

   schema.addColumn( "b", { Type: "int32", ReadDefault: 1, WriteDefault: 2 } );
   schema.addColumn( "c", { Type: "int32", ReadDefault: 20, WriteDefault: 30 } );

   docs = [];
   for( var i = 50; i < 100; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 2, c: 30 } );
      expPrimalResult.push( { a: i, b: 2, c: 30 } );
   }
   dbcl.insert( docs );

   // 匹配非贴源b字段，更新非贴源c数据
   dbcl.upsert( { $set: { c: 2 } }, { b: 1 } );

   // 匹配非贴源b字段，更新贴源a数据
   dbcl.upsert( { $set: { a: 30 } }, { b: 1 } );

   // 校验内部结构
   var expInternalColumnDef = {
      "a": {},
      "b": {
         "ReadDefault": 1,
         "WriteDefault": 2
      },
      "c": {
         "ReadDefault": 20,
         "WriteDefault": 30
      }
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, testConf.clName, sel, expResult, expPrimalResult, expInternalColumnDef );
}