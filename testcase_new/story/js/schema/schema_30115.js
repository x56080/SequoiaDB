/******************************************************************************
 * @Description   : seqDB-30115：集合不存在数据开启内部模式
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.21
 * @LastEditTime  : 2023.02.21
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30115";
testConf.schemaDef = { "c": { Type: "int32", WriteDefault: 10 } };
testConf.clName = COMMCLNAME + "_30115";
main( test );
function test ( testPara )
{
   // 开启内部模式
   var cl = testPara.testCL;
   var option = { "EnableInfoSchema": true };
   cl.alter( option );
   var ret = db.snapshot( SDB_SNAP_CATALOG, { "Name": COMMCSNAME + "." + testConf.clName } );
   var actAttr = ret.current().toObj().AttributeDesc;
   if( actAttr.indexOf( "EnableInfoSchema" ) === -1 )
   {
      throw new Error( "cl failed to open internal schema!" );
   }

   // 绑定外部模式
   cl.addSchema( testConf.schemaName );
   checkAddSchema( db, COMMCSNAME, testConf.clName, testConf.schemaName );

   // 插入数据
   var doc = [];
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i, b: i } );
      expResult.push( { a: i, b: i, c: 10 } );
   }
   cl.insert( doc );

   var actResult = cl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
}

