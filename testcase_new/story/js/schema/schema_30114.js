/******************************************************************************
 * @Description   : seqDB-30114:外部模式添加的字段
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.03.02
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30114";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "string", WriteDefault: "infoSchema" } };
testConf.clName = COMMCLNAME + "_30114";

main( test );
function test ()
{
   var schema = db.getSchema( testConf.schemaName );

   // 添加外部模式已存在的字段
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.addColumn( "a", { Type: "int32" } );
   } )

   // 外部模式添加一个不存在的字段，检查外部模式属性
   schema.addColumn( "c", { Type: "date" } );
   var expectedColumnDef = {
      "a": {
         "Type": "int32",
         "Restrict": 0,
         "RestrictDesc": ""
      },
      "b": {
         "Type": "string",
         "WriteDefault": "infoSchema",
         "Restrict": 0,
         "RestrictDesc": ""
      },
      "c": {
         "Type": "date",
         "Restrict": 0,
         "RestrictDesc": ""
      }
   };
   checkColumnDef( db, testConf.schemaName, expectedColumnDef );

   // 外部模式添加字段不指定Type
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      schema.addColumn( "d", {} );
   } )
}