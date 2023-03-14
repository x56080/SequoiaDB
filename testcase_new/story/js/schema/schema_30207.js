/******************************************************************************
 * @Description   : seqDB-30207:$rename修改外部模式字段，字段读默认值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.27
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   // 测试插入数据后开启内部模式
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { ReplSize: 0 } );
      },
      function( dbcl )
      {
         dbcl.alter( { EnableInfoSchema: true } );
      } );

   // 测试开启内部模式后插入数据
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
   var clName = "cl_30207";
   var schemaName = "schema_30207";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var schemaDef = {
      "a": { Type: "int32", ReadDefault: 10 }, "b": { Type: "string", ReadDefault: "test" }
   }

   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   var docs = [];
   var expResult = [];
   var priexpResult = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: "infoSchema" } );
      expResult.push( { a: i, c: "infoSchema", b: "test" } );

      priexpResult.push( { a: i, c: "infoSchema" } );
   }
   dbcl.insert( docs );

   func2( dbcl );
   dbcl.addSchema( schemaName );

   // 使用 $rename 更新 b 字段为 c
   // dbcl.update( { $rename: { "b": "c" } } );

   // 校验数据
   // expResult.sort( sortBy( 'a' ) );
   // var actResult = dbcl.find().sort( { a: 1 } );
   // commCompareResults( actResult, expResult );
   // priexpResult.sort( sortBy( 'a' ) );
   // var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   // commCompareResults( actResult, priexpResult );

   // // 修改后外部模式字段无变化
   // var expectedColumnDef = {
   //    "a": {
   //       "Type": "int32",
   //       "ReadDefault": 10,
   //       "Restrict": 0,
   //       "RestrictDesc": ""
   //    },
   //    "b": {
   //       "Type": "string",
   //       "ReadDefault": "test",
   //       "Restrict": 0,
   //       "RestrictDesc": ""
   //    }
   // };
   // checkColumnDef( db, schemaName, expectedColumnDef );

   // // 修改后重新插入数据,按外部模式指定字段生效
   // docs = [];
   // for( var i = 100; i < 200; i++ )
   // {
   //    docs.push( { a: i, b: "infoSchema" } );
   //    expResult.push( { a: i, b: "infoSchema" } );

   //    priexpResult.push( { a: i, b: "infoSchema" } );
   // }
   // dbcl.insert( docs );

   // // 校验数据
   // expResult.sort( sortBy( 'a' ) );
   // var actResult = dbcl.find().sort( { a: 1 } );
   // commCompareResults( actResult, expResult );
   // priexpResult.sort( sortBy( 'a' ) );
   // var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   // commCompareResults( actResult, priexpResult );

   commDropCL( db, COMMCSNAME, clName );
}