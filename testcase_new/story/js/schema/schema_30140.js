/******************************************************************************
 * @Description   : seqDB-30140：集合所有集合均包含外部模式定义字段，字段设置读写默认值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30140";
testConf.schemaDef = {
   "a": { Type: "int32", WriteDefault: 10, ReadDefault: 20 },
   "b": { Type: "decimal", WriteDefault: { $decimal: "123.456" }, ReadDefault: { $decimal: "1.001" } }
};
testConf.clName = COMMCLNAME + "_30140";
testConf.clOpt = { EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   // 插入数据
   var doc = [];
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i, b: 1.01, c: i } );
      expResult.push( { a: i, b: 1.01, c: i } );
   }
   dbcl.insert( doc );

   // 绑定外部模式
   dbcl.alter( { EnableInfoSchema: true } );
   dbcl.addSchema( testConf.schemaName );

   var actResult = dbcl.find().sort( { c: 1 } );
   commCompareResults( actResult, doc );
   actResult = dbcl.find().sort( { c: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, doc );

   // 插入数据
   doc = [];
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { a: i, b: 1.01, c: i } );
      expResult.push( { a: i, b: 1.01, c: i } );
   }
   dbcl.insert( doc );

   var actResult = dbcl.find().sort( { c: 1 } );
   commCompareResults( actResult, expResult );
   actResult = dbcl.find().sort( { c: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // 插入数据不包含默认值字段
   doc = [];
   for( var i = 40; i < 60; i++ )
   {
      doc.push( { c: i } );
      expResult.push( { a: 10, b: { "$decimal": "123.456" }, c: i } );
   }
   dbcl.insert( doc );

   var actResult = dbcl.find().sort( { c: 1 } );
   commCompareResults( actResult, expResult );
   actResult = dbcl.find().sort( { c: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   var expInternalColumnDef = {
      "a": {
         "ReadDefault": 20,
         "WriteDefault": 10
      },
      "b": {
         "ReadDefault": {
            "$decimal": "1.001"
         },
         "WriteDefault": {
            "$decimal": "123.456"
         }
      },
      "c": {}
   };

   checkInternalSchema( dbcl, expInternalColumnDef );
   commDropCL( db, COMMCSNAME, testConf.clName );
}