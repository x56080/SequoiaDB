/******************************************************************************
 * @Description   : seqDB-30119:已绑定外部模式的集合关闭内部模式
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30119";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "double", WriteDefault: 2.12 } };
testConf.clName = COMMCLNAME + "_30119";
testConf.csName = COMMCSNAME + "_30119";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( testPara )
{
   dbcl = testPara.testCL;
   dbcl.addSchema( testConf.schemaName );
   // 集合插入数据，均为外部模式中存在的字段
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: 3.17 } );
   }
   dbcl.insert( docs );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.alter( { EnableInfoSchema: false } );
   } )

   dbcl.truncate();

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.alter( { EnableInfoSchema: false } );
   } )
}