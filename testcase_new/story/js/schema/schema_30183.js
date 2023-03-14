/******************************************************************************
 * @Description   : seqDB-30183:分区键有索引，外部模式增加分区键字段
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.03.01
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30183";
testConf.schemaDef = { "b": { Type: "int32" }, "c": { Type: "string" } };
testConf.clName = COMMCLNAME + "_30183";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "AutoSplit": true, EnableInfoSchema: true };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   dbcl.addSchema( testConf.schemaName );

   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { b: i, c: "test" } );
      expResult.push( { b: i, c: "test" } );
      expPrimalResult.push( { b: i, c: "test" } );
   }
   dbcl.insert( docs );

   var schema = db.getSchema( testConf.schemaName );
   schema.addColumn( "a", { Type: "int32", ReadDefault: 10 } );
}