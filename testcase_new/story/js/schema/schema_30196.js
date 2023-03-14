/******************************************************************************
 * @Description   : seqDB-30196:集合存在贴源数据执行split
 * @Author        : liuli
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.28
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
   var clName = "cl_30196";
   var schemaName = "schema_30196";

   commDropCL( db, COMMCSNAME, clName );
   commDropSchema( db, schemaName );

   var groupNames = commGetDataGroupNames( db );
   var writeDefault = { "$date": "2023-02-24" };
   var schemaDef = { b: { Type: "int32" }, c: { Type: "date", WriteDefault: writeDefault } };
   commCreateSchema( db, schemaName, schemaDef );

   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = func1( dbcs, clName, groupNames[0] );

   // 插入数据，全部为外部模式中包含的字段
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i, c: { "$date": "2023-02-25" } } );
   }
   dbcl.insert( docs );

   func2( dbcl, schemaName );

   // 切分部分数据到group2
   dbcl.split( groupNames[0], groupNames[1], 30 );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   // 再次切分部分数据到group2
   dbcl.split( groupNames[0], groupNames[1], 50 );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   commDropCL( db, COMMCSNAME, clName );
}