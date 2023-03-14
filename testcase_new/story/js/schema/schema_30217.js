/******************************************************************************
 * @Description   : seqDB-30217：findAndModify时指定flag
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.02.28
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30217";
testConf.schemaDef = { "b": { Type: "string", WriteDefault: "write" } };
testConf.clName = COMMCLNAME + "_30217";
testConf.clOpt = { EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   testPara.testCL.addSchema( testConf.schemaName );
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // 插入数据
   var docs = [];
   for( var i = 0; i < 20; i++ )
   {
      docs.push( { a: i, b: "insertString" } );
   }
   testPara.testCL.insert( docs );

   docs = [];
   for( var i = 20; i < 40; i++ )
   {
      docs.push( { a: i, b: "insertString", c: i } );
   }
   testPara.testCL.insert( docs );

   // 外部模式新增字段设置读默认值
   testPara.testSchema.addColumn( "c", { Type: "int32", ReadDefault: "10" } );

   // findAndUpdate
   // testPara.testCL.find().flags( SDB_FLG_QUERY_PRIMAL_DATA ).update( { $set: { c: 100 } } );
}
