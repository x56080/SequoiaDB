/******************************************************************************
 * @Description   : seqDB-30131：集合绑定的外部模式中包含_id字段
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.25
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30131";
testConf.schemaDef = { "_id": { Type: "oid", WriteDefault: { "$oid": "63f76b6e74ea9b5455d4f98a" } } };
// testConf.schemaDef = { "_id": { Type: "oid", WriteDefault: { "$oid": "63f76b6e74ea9b5455d4f98a" }, ReadDefault: { "$oid": "63f76b6e74ea9b5455d4f98a" } } };
testConf.clName = COMMCLNAME + "_30131";
testConf.clOpt = { EnableInfoSchema: true };
main( test );

function test ( testPara )
{
   testPara.testCL.addSchema( testConf.schemaName );

   // 写入数据不包含字段：_id
   var doc = [];
   for( var i = 0; i < 20; i++ )
   {
      doc.push( { a: i } );
   }
   testPara.testCL.insert( doc );

   var cursor = testPara.testCL.find().sort( { a: 1 } );
   while( cursor.next() )
   {
      var actRecord = cursor.current().toObj();
      assert.notEqual( { "$oid": "63f76b6e74ea9b5455d4f98a" }, actRecord._id );
   }
   cursor.close();
}