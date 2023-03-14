/******************************************************************************
 * @Description   : seqDB-30147:外部模式增加多个字段，字段数量超过最大值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.06
 * @LastEditTime  : 2023.03.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

// main( test );

function test ()
{
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { ReplSize: 0 } );
      },
      function( dbcl )
      {
         dbcl.alter( { EnableInfoSchema: true } );
      } );

   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { EnableInfoSchema: true, ReplSize: 0 } );
      },
      function()
      { } );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30345";
   var schemaName = "schema_30345";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   // 创建外部模式
   var schemaDef = {
      "a": { Type: "int32" }, "b": { Type: "date" }, "c": { Type: "string" }
   };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var docs = [];
   for( var i = 0; i < 10; i++ )
   {
      docs.push( { a: i, b: { "$date": "2022-10-01" }, c: "test" } );
   }
   dbcl.insert( docs );

   func2( dbcl );
   dbcl.addSchema( schemaName );

   // throw new Error( "bb" );
   // 外部模式新增多个字段，字段总长度累计超过64kb
   var text1 = "";
   var text2 = "";
   for( var i = 0; i < 40 * 1024; i++ )
   {
      text1 += "d";
      text2 += "e";
   }
   schema.addColumn( text1, { Type: "string", ReadDefault: 20, WriteDefault: 20 } );
   schema.addColumn( text2, { Type: "string", ReadDefault: 30, WriteDefault: 40 } );

   commDropCL( db, COMMCSNAME, clName );
}