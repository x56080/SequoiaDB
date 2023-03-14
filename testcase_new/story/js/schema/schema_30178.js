/******************************************************************************
 * @Description   : seqDB-30178:删除数据匹配外部模式中存在的字段
 * @Author        : liuli
 * @CreateTime    : 2023.02.23
 * @LastEditTime  : 2023.02.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30178";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: 101 } };
testConf.clName = COMMCLNAME + "_30178";
testConf.clOpt = { EnableInfoSchema: true };

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var schema = testPara.testSchema;
   var writeDefault = ["writeDefault"];
   var readDefault = ["readDefault"];

   dbcl.addSchema( testConf.schemaName );

   // 插入数据，包含外部模式中所有字段
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 外部模式增加字段，设置读写默认值
   var columnDef = { Type: "array", WriteDefault: writeDefault, ReadDefault: readDefault };
   schema.addColumn( "c", columnDef );

   // 再次插入数据
   var expResult = [];
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i, b: i, c: [writeDefault + i] } );
      if( i < 150 )
      {
         expResult.push( { a: i, b: i, c: [writeDefault + i] } );
      }
   }
   dbcl.insert( docs );

   // 匹配贴源记录，贴源字段删除数据
   dbcl.remove( { a: { $gte: 150 } } );

   // 匹配非贴源集合，非贴源字段删除数据
   dbcl.remove( { c: readDefault } );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );
}