/******************************************************************************
 * @Description   : seqDB-30104：已绑定外部模式的表作为子表挂载
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "mainCL_30104";
   var subCLName1 = "subCL_30104_1";
   var subCLName2 = "subCL_30104_2";
   var schemaName1 = "schemaName_30104_1";
   var schemaName2 = "schemaName_30104_2";
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropSchema( db, schemaName1 );
   commDropSchema( db, schemaName2 );

   // 创建多个外部模式
   var schemaDef = { "a": { Type: "int32" } };
   commCreateSchema( db, schemaName1, schemaDef );
   commCreateSchema( db, schemaName2, schemaDef );

   // 创建主表和子表
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   var subcl1 = commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );

   // 子表1绑定外部模式
   subcl1.addSchema( schemaName1 );

   // 挂载子表
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   } )
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 0 }, UpBound: { a: 100 } } );

   // 主表绑定外部模式
   maincl.addSchema( schemaName2 );

   //挂载子表
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   } )

   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropCL( db, COMMCSNAME, mainCLName );
}

