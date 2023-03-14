/******************************************************************************
 * @Description   : seqDB-30118:未绑定外部模式存在数据的集合关闭内部模式
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.03.02
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_30118";
testConf.clOpt = { EnableInfoSchema: true };

main( test );
function test ( testPara )
{
   dbcl = testPara.testCL;

   // 关闭内部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.alter( { EnableInfoSchema: false } );
   } )

   // 集合插入数据
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: "testInfoSchema" } );
   }
   dbcl.insert( docs );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.alter( { EnableInfoSchema: false } );
   } )

   dbcl.remove();
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.alter( { EnableInfoSchema: false } );
   } )
}