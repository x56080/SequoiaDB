/******************************************************************************
 * @Description   : seqDB-30226:dropColumn接口参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.02.28
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30226";
testConf.schemaDef = { "b": { Type: "int32" }, "c": { Type: "string" } };

main( test );
function test ()
{
   var schema = db.getSchema( testConf.schemaName );
   schema.dropColumn( "b" );

   var expectedColumnDef = {
      "c": {
         "Type": "string",
         "Restrict": 0,
         "RestrictDesc": ""
      }
   };
   checkColumnDef( db, testConf.schemaName, expectedColumnDef );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.dropColumn( 1111 );
   } )
}