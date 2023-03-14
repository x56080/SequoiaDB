/******************************************************************************
 * @Description   : seqDB-30111:版本: 1 :: 创建外部模式字段定义不正确
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.22
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
main( test );
function test ()
{
   var schemaName = "schema_30111";
   var schemaDef = { "a": { Type: "int32", ReadDefault: 1 }, "b": { Type: "int" }, "b": { Type: "string" } };
   commDropSchema( db, schemaName );

   commCreateSchema( db, schemaName, schemaDef );

   var expectedColumnDef = {
      "a": {
         "Type": "int32",
         "ReadDefault": 1,
         "Restrict": 0,
         "RestrictDesc": ""
      },
      "b": {
         "Type": "string",
         "Restrict": 0,
         "RestrictDesc": ""
      }
   };
   checkColumnDef( db, schemaName, expectedColumnDef );

   var schemaDef = { "a": { ReadDefault: 1 } };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSchema( schemaName, schemaDef );
   } )

   commDropSchema( db, schemaName );
}