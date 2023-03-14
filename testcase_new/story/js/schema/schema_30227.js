/******************************************************************************
 * @Description   : seqDB-30227:schema.alter接口参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30227";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "string" } };

main( test );
function test ()
{
   var schema = db.getSchema( testConf.schemaName );
   schema.alter( { "StrictMode": true } );
   var ret = db.list( SDB_LIST_SCHEMAS, { Name: testConf.schemaName } );
   var actStrictModeDef = ret.current().toObj().StrictMode;
   var expStrictModeDef = true;
   assert.equal( actStrictModeDef, expStrictModeDef );

   // schema.alter( "{}" );

   // schema.alter( { "Name": "bb" } );
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      schema.alter();
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.alter( { "StrictMode": "test" } );
   } )
}