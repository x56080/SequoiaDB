
/******************************************************************************
 * @Description   : seqDB-30121:绑定外部模式的主表，关闭内部模式
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName1 = "mainCL_30121_1";
   var mainCLName2 = "mainCL_30121_2";
   var subCLName1 = "subCL_30121_1";
   var subCLName2 = "subCL_30121_2";
   var schemaName1 = "schemaName_30121_1";
   var schemaName2 = "schemaName_30121_2";
   commDropCL( db, COMMCSNAME, mainCLName1 );
   commDropCL( db, COMMCSNAME, mainCLName2 );
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );

   // 创建两个主表，其中一个主表需要挂载子表
   var maincl1 = commCreateCL( db, COMMCSNAME, mainCLName1, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   var maincl2 = commCreateCL( db, COMMCSNAME, mainCLName2, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl1.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   maincl1.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   // 创建两个外部模式
   var schemaDef = { "a": { Type: "int32" } };
   commCreateSchema( db, schemaName1, schemaDef );
   commCreateSchema( db, schemaName2, schemaDef );

   // 主表挂载外部模式
   maincl1.addSchema( schemaName1 );
   maincl2.addSchema( schemaName2 );

   // 主表关闭内部模式
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl1.alter( { "EnableInfoSchema": false } );
   } )
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl2.alter( { "EnableInfoSchema": false } );
   } )

   commDropCL( db, COMMCSNAME, mainCLName1 );
   commDropCL( db, COMMCSNAME, mainCLName2 );
}

