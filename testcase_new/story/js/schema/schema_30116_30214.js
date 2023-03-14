/******************************************************************************
 * @Description   : seqDB-30116:集合存在数据开启内部模式
 *                : seqDB-30214:replsize为0的集合使用外部模式
 * @Author        : liuli
 * @CreateTime    : 2023.02.21
 * @LastEditTime  : 2023.02.27
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.schemaName = COMMSCHEMANAME + "_30116_30214";
testConf.schemaDef = { "a": { Type: "int32" }, "b": { Type: "int32", WriteDefault: 10 } };
testConf.clName = COMMCLNAME + "_30116_30214";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   // 插入一部分数据
   var expResult = [];
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i } );
   }
   dbcl.insert( docs );

   // 集合开启内部模式
   dbcl.alter( { EnableInfoSchema: true, ReplSize: 0 } );

   // 校验开启内部模式成功
   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: COMMCSNAME + "." + testConf.clName } );
   var expAttributeDesc = "Compressed | EnableInfoSchema";
   while( cursor.next() )
   {
      var attributeDesc = cursor.current().toObj().AttributeDesc;
      assert.equal( attributeDesc, expAttributeDesc );
   }
   cursor.next();

   // 绑定外部模式增加字段b包含写默认值
   dbcl.addSchema( testConf.schemaName );

   // 插入数据不包含b字段
   docs = [];
   for( var i = 100; i < 200; i++ )
   {
      docs.push( { a: i } );
      expResult.push( { a: i, b: 10 } );
   }
   dbcl.insert( docs );

   // 贴源、非贴源校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );

   // seqDB-30214 设置会话属性访问备节点
   db.setSessionAttr( { PreferredInstance: "S" } );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expResult );
}