/******************************************************************************
 * @Description   : seqDB-30198:集合存在贴源数据执行100% split
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test ()
{
   // 开启内部模式插入数据后绑定外部模式
   testSchema(
      function( dbcs, clName, groupName )
      {
         var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, Group: groupName, EnableInfoSchema: true } );
         return dbcl;
      },
      function( dbcl, schemaName )
      {
         dbcl.addSchema( schemaName );
      } );

   // 插入数据后开启内部模式绑定外部模式
   testSchema(
      function( dbcs, clName, groupName )
      {
         return dbcs.createCL( clName, { ShardingKey: { a: 1 }, Group: groupName } );
      },
      function( dbcl, schemaName )
      {
         dbcl.alter( { EnableInfoSchema: true } );
         dbcl.addSchema( schemaName );
      } );
}

function testSchema ( func1, func2 )
{
   var clName = "cl_30199";
   var schemaName = "schema_30199";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var groupNames = commGetDataGroupNames( db );
   var schemaDef = { b: { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, groupNames[0] );

   // 插入数据，全部为外部模式中包含的字段
   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { b: i } );
      expResult.push( { b: i } );
      expPrimalResult.push( { b: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 再次插入数据
   docs = [];
   for( var i = 1000; i < 2000; i++ )
   {
      docs.push( { a: i, b: i, c: { "string": "test" } } );
      expResult.push( { a: i, b: i, c: { "string": "test" } } );
      expPrimalResult.push( { a: i, b: i, c: { "string": "test" } } );
   }
   dbcl.insert( docs );

   // 100%切分数据到group2
   dbcl.split( groupNames[0], groupNames[1], 100 );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );

   commDropCL( db, COMMCSNAME, clName );
}