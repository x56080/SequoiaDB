/******************************************************************************
 * @Description   : seqDB-30180:非贴源字段创建唯一索引
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30180";
testConf.schemaDef = { "a": { Type: "string" } };
testConf.clName = COMMCLNAME + "_30180";
testConf.clOpt = { EnableInfoSchema: true }

main( test );
function test ( testPara )
{
   var schema = db.getSchema( testConf.schemaName );
   var dbcl = testPara.testCL;
   var indexName = "index_30180";

   // 检查外部模式shema
   checkColumnDef( db, testConf.schemaName, testConf.schemaDef );

   // 集合绑定外部模式
   dbcl.addSchema( testConf.schemaName );

   // 写入数据，包含外部模式所有字段
   var docs = [];
   for( var i = 0; i < 50; i++ )
   {
      docs.push( { a: "testinfoSchema1" } );
   }
   dbcl.insert( docs );

   // 新增外部模式字段，设置读写默认值
   schema.addColumn( "b", { Type: "int32", ReadDefault: 10, WriteDefault: 20 } );

   docs = [];
   for( var i = 50; i < 100; i++ )
   {
      docs.push( { a: "testinfoSchema2", b: i } );
   }
   dbcl.insert( docs );

   // 新增字段创建唯一索引
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.createIndex( indexName, { b: 1 }, { 'Unique': true } );
   } )
}