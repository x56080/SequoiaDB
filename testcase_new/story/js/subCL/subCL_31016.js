/******************************************************************************
 * @Description   : seqDB-31016:挂载子表长度检查
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.03.31
 * @LastEditTime  : 2023.03.31
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ( testPara )
{
   // 子表全名（cs.cl）长度大于0个字节，小于127字节
   var mainClName = CHANGEDPREFIX + "mainClName_31016";
   var subClName = CHANGEDPREFIX + "subClName_31016";
   commDropCL( db, COMMCSNAME, subClName, true, true );
   commDropCL( db, COMMCSNAME, mainClName, true, true );
   var cs = testPara.testCS;
   var mainCL = cs.createCL( mainClName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   cs.createCL( subClName, { ShardingKey: { a: 1 } } );
   mainCL.attachCL( COMMCSNAME + "." + subClName, { LowBound: { a: { $minKey: 1 } }, UpBound: { a: 50 } } );
   commDropCL( db, COMMCSNAME, subClName, true, true );
   commDropCL( db, COMMCSNAME, mainClName, true, true );

   // 子表全名（cs.cl）长度大于127个字节，小于255字节
   var name = new Array( 128 );
   subClName = name.join( "b" );
   var mainCL = cs.createCL( mainClName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   cs.createCL( subClName, { ShardingKey: { a: 1 } } );
   mainCL.attachCL( COMMCSNAME + "." + subClName, { LowBound: { a: { $minKey: 1 } }, UpBound: { a: 50 } } );
   commDropCL( db, COMMCSNAME, subClName, true, true );

   // 子表全名（cs.cl）长度大于255字节（主要是校验错误码是否正确）
   // a.cs长度127，cl长度超过127
   name = new Array( 127 );
   csName = name.join( "a" );
   commDropCS( db, csName );
   var cs = db.createCS( csName );
   name = new Array( 200 );
   subClName = name.join( "b" );
   var mainCL = cs.createCL( mainClName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      mainCL.attachCL( csName + "." + subClName, { LowBound: { a: { $minKey: 1 } }, UpBound: { a: 50 } } );
   } );
   commDropCS( db, csName );
   // b.cs长度超过127，cl长度为127
   name = new Array( 200 );
   csName = name.join( "a" );
   name = new Array( 127 );
   subClName = name.join( "b" );
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      mainCL.attachCL( csName + "." + subClName, { LowBound: { a: { $minKey: 1 } }, UpBound: { a: 50 } } );
   } );
   // c.cs长度超过127，cl长度超过127
   name = new Array( 200 );
   csName = name.join( "a" );
   subClName = name.join( "b" );
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      mainCL.attachCL( csName + "." + subClName, { LowBound: { a: { $minKey: 1 } }, UpBound: { a: 50 } } );
   } );
}