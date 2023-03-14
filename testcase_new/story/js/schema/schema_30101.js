/******************************************************************************
 * @Description   : seqDB-30101:主表开启内部模式
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.03.02
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30101";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "string", WriteDefault: "infoSchema" } };

//main( test );
function test ()
{
   var mainCLName1 = "maincl_30101_1";
   var mainCLName2 = "maincl_30101_2";
   var subCLName1 = "subcl_30101_1";
   var subCLName2 = "subcl_30101_2";

   var maincl1 = commCreateCL( db, COMMCSNAME, mainCLName1, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, EnableInfoSchema: true } );
   // 创建子表subcl1，不开启内部模式
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl1.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 5 } } );
   } )

   var maincl2 = commCreateCL( db, COMMCSNAME, mainCLName2, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true, EnableInfoSchema: true } );
   maincl2.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 5 } } );
   maincl2.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 5 }, UpBound: { a: 10 } } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl2.addSchema( testConf.schemaName );
   } )

   // 主表maincl2开启内部模式
   maincl2.alter( { EnableInfoSchema: true } );

   // 检查主表maincl2和所挂载的子表内部模式相关属性
   var expEnableInfoSchema = "EnableInfoSchema";
   var cur = sdb.snapshot( SDB_SNAP_CATALOG, { "Name": COMMCSNAME + "." + mainCLName2 } );
   var tmpcur = cur.current().toObj()["AttributeDesc"];
   var actEnableInfoSchema = tmpcur.split( "|" )[1];
   assert.equal( expEnableInfoSchema, actEnableInfoSchema );
   var cur1 = sdb.snapshot( SDB_SNAP_CATALOG, { "Name": COMMCSNAME + "." + subCLName1 } );
   var tmpcur1 = cur1.current().toObj()["AttributeDesc"];
   var actEnableInfoSchema1 = tmpcur1.split( "|" )[1];
   assert.equal( expEnableInfoSchema, actEnableInfoSchema1 );
   var cur2 = sdb.snapshot( SDB_SNAP_CATALOG, { "Name": COMMCSNAME + "." + subCLName2 } );
   var tmpcur2 = cur2.current().toObj()["AttributeDesc"];
   var actEnableInfoSchema2 = tmpcur2.split( "|" )[1];
   assert.equal( expEnableInfoSchema, actEnableInfoSchema2 );

   maincl2.addSchema( testConf.schemaName );
   // 检查绑定外部模式是否成功
   checkAddSchema( db, COMMCSNAME, mainCLName2, testConf.schemaName );
}