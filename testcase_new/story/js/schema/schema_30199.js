/******************************************************************************
 * @Description   : seqDB-30199:集合存在贴源/非贴源数据执行100% split
 * @Author        : liuli
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.03.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

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
   var readDefault = { "$timestamp": "2023-02-24-09.15.21.241523" };

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var groupNames = commGetDataGroupNames( db );
   var schemaDef = { b: { Type: "int32" } };
   var schema = commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, groupNames[0] );

   // 插入数据，全部为外部模式中包含的字段
   var docs = [];
   var expResult = [];
   var expPrimalResult = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { a: i, b: i, c: readDefault } );
      expPrimalResult.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 外部模式增加字段，设置读默认值
   schema.addColumn( "c", { Type: "timestamp", ReadDefault: readDefault } );

   // 再次插入数据包含全部字段
   docs = [];
   for( var i = 1000; i < 2000; i++ )
   {
      docs.push( { a: i, b: i, c: { "$timestamp": "2023-02-24-14.15.21.241523" } } );
      expResult.push( { a: i, b: i, c: { "$timestamp": "2023-02-24-14.15.21.241523" } } );
      expPrimalResult.push( { a: i, b: i, c: { "$timestamp": "2023-02-24-14.15.21.241523" } } );
   }
   dbcl.insert( docs );

   // 100%切分数据到group2
   dbcl.split( groupNames[0], groupNames[1], 100 );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );
   var expInternalColumnDef = { a: {}, b: {}, c: { ReadDefault: readDefault } };
   checkInternalSchema( dbcl, expInternalColumnDef );

   commDropCL( db, COMMCSNAME, clName );
}