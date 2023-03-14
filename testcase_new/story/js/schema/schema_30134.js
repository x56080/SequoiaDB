/******************************************************************************
 * @Description   : seqDB-30134:集合部分数据包含外部模式字段，外部模式字段设置写默认值
 *                  seqDB-30096:创建集合绑定不存在的外部模式
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.21
 * @LastEditTime  : 2023.02.23
 * @LastEditors   : Cheng Jingjing
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
      }
   );

   // 测试开启内部模式后插入数据
   testSchema(
      function( dbcs, clName )
      {
         return dbcs.createCL( clName, { EnableInfoSchema: true, ReplSize: 0 } );
      },
      function() { }
   );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30134";
   var schemaName = "schema_30134";
   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName );

   // cl add a schema which is not exist
   assert.tryThrow( SDB_SCHEMA_NOT_EXIST, function()
   {
      dbcl.addSchema( "schema" );
   } )

   // insert data
   var doc = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i, b: i, c: "test" + i } );
   }
   for( var i = 20; i < 40; i++ )
   {
      doc.push( { a: i, c: "test" + i } );
   }
   dbcl.insert( doc );

   // create schema
   var columnDef = { Type: "int32", WriteDefault: 100 };
   db.createSchema( schemaName, { b: columnDef } );
   checkColumnDef( db, schemaName, { b: columnDef } );

   // add schema
   func2( dbcl );
   dbcl.addSchema( schemaName );
   checkAddSchema( db, COMMCSNAME, clName, schemaName );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, doc );

   // insert record inculde b
   var record = { a: 44, b: 44, c: 44 };
   doc.push( record );
   dbcl.insert( record );
   actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, doc );

   // insert record exclude b
   record = { a: 55, c: 55 };
   doc.push( { a: 55, b: 100, c: 55 } );
   dbcl.insert( record );
   actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, doc );

   var expInternalColumnDef = {
      "b": {
         "WriteDefault": 100
      },
      "a": {},
      "c": {}
   };
   checkInternalSchema( dbcl, expInternalColumnDef );

   var sel = { a: 1 };
   checkConsistence( db, COMMCSNAME, clName, sel, doc );

   commDropCL( db, COMMCSNAME, clName );
}
