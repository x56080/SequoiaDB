/******************************************************************************
 * @Description   : seqDB-30228:collection.alter接口参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "string" } };
testConf.clName = COMMCLNAME + "_30228";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   dbcl.alter( { EnableInfoSchema: true } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.alter( { EnableInfoSchema: "test" } );
   } )
}