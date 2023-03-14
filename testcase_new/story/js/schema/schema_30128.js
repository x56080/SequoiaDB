/******************************************************************************
 * @Description   : seqDB-30128：不存在数据的集合绑定外部模式，外部模式包含多个字段
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30128";
testConf.schemaDef = { "a": { Type: "int32" }, "date": { Type: "date", ReadDefault: { "$date": "2012-01-01" } }, "age": { Type: "int32", WriteDefault: 20 }, "name": { Type: "string", ReadDefault: "read", WriteDefault: "write" } };
testConf.clName = COMMCLNAME + "_30128";
testConf.clOpt = { EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   testPara.testCL.addSchema( testConf.schemaName );
   // 插入数据
   var doc = [];
   var expResult = [];
   var primalResult = [];
   for( var i = 0; i < 20; i++ )
   {
      // 包含部分字段
      doc.push( { a: i } );
      expResult.push( { a: i, date: { "$date": "2012-01-01" }, age: 20, name: "write" } );
      primalResult.push( { a: i, age: 20, name: "write" } );
      // 包含全部字段
      doc.push( { a: i + 20, date: { "$date": "2023-01-01" }, age: 20, name: "name" } );
      expResult.push( { a: i + 20, date: { "$date": "2023-01-01" }, age: 20, name: "name" } );
      primalResult.push( { a: i + 20, date: { "$date": "2023-01-01" }, age: 20, name: "name" } );
      // 包含外部模式没有的字段
      doc.push( { a: i + 40, b: i } )
      expResult.push( { a: i + 40, b: i, date: { "$date": "2012-01-01" }, age: 20, name: "write" } );
      primalResult.push( { a: i + 40, b: i, age: 20, name: "write" } );
   }
   testPara.testCL.insert( doc );

   expResult.sort( sortBy( 'a' ) );
   var actResult = testPara.testCL.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   primalResult.sort( sortBy( 'a' ) );
   actResult = testPara.testCL.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, primalResult );
}
