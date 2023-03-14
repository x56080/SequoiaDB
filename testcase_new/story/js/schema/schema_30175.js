/******************************************************************************
 * @Description   : seqDB-30175:查询数据匹配外部模式中存在的字段
 * @Author        : liuli
 * @CreateTime    : 2023.02.23
 * @LastEditTime  : 2023.02.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30175";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: 101 } };
testConf.clName = COMMCLNAME + "_30175";
testConf.clOpt = { EnableInfoSchema: true };

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var schema = testPara.testSchema;
   var writeDefault = 10.1;
   var readDefault = 20.1;

   dbcl.addSchema( testConf.schemaName );

   // 插入数据，包含外部模式中所有字段
   var expResult2 = [];
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult2.push( { a: i, b: i, c: 20.1 } );
   }
   dbcl.insert( docs );

   // 外部模式增加字段，设置读写默认值
   var columnDef = { Type: "double", WriteDefault: writeDefault, ReadDefault: readDefault };
   schema.addColumn( "c", columnDef );

   // 再次插入数据
   var expResult = [];
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
      expResult.push( { a: i, b: i, c: i } );
   }
   dbcl.insert( docs );
   expResult.push( { a: 99, b: 99, c: readDefault } );

   // 匹配贴源字段查询数据，指定贴源字段排序
   expResult.sort( sortBy( "a" ) );
   var actResult = dbcl.find( { a: { $gte: 99 } } ).sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 匹配贴源字段查询数据，指定非贴源字段排序
   expResult.sort( sortBy( "c" ) );
   var actResult = dbcl.find( { a: { $gte: 99 } } ).sort( { c: 1 } );
   commCompareResults( actResult, expResult );

   // 匹配非贴源字段查询，指定贴源字段排序
   expResult2.sort( sortBy( "a" ) );
   var actResult = dbcl.find( { c: readDefault } ).sort( { a: 1 } );
   commCompareResults( actResult, expResult2 );
}