/******************************************************************************
 * @Description   : seqDB-30201:集合原组合目标组内部模式不一致执行split
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.28
 * @LastEditTime  : 2023.03.06
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test ()
{
   var csName = "cs_30201";
   var clName = "cl_30201";
   var schemaName = "schema_30201";
   var domainName = CHANGEDPREFIX + "_domain_30201";

   commDropCS( db, csName );
   commDropSchema( db, schemaName );
   commDropDomain( db, domainName );

   // 创建外部模式
   var schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32" } };
   commCreateSchema( db, schemaName, schemaDef );

   var groupNames = commGetDataGroupNames( db );
   commDropDomain( db, domainName );
   commCreateDomain( db, domainName, [groupNames[0], groupNames[1]], { AutoSplit: true } );
   db.createCS( csName, { Domain: domainName } )
   var clOption = { ShardingKey: { a: 1 }, ShardingType: 'hash', EnableInfoSchema: true };
   var dbcl = commCreateCL( db, csName, clName, clOption, true, true );
   dbcl.addSchema( schemaName );

   var docs = [];
   for( var i = 0; i < 50; i++ )
   {
      docs.push( { a: i, b: "test", c: i } );
   }
   for( var i = 50; i < 100; i++ )
   {
      docs.push( { a: i, b: "infoSchema" } );
   }
   dbcl.insert( docs );

   // 切分部分数据到group2
   dbcl.split( groupNames[0], groupNames[1], 30 );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, docs );

   commDropCS( db, csName );
   commDropDomain( db, domainName );
}