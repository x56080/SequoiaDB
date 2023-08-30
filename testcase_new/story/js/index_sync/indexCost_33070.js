/******************************************************************************
 * @Description   : seqDB-33070:optstartcostlimit参数验证
 * @Author        : wuyan
 * @CreateTime    : 2022.08.30
 * @LastEditTime  : 2023.08.30
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_index33070";

main( test );
function test ()
{
   var cl = testPara.testCL;
   cl.createIndex( "a", { "a": 1 } );

   var data = [];
   for( i = 0; i < 100000; ++i )
   {
      data.push( { a: i, b: Math.round( ( Math.random() * 100000 ) ), c: i, d: i } )
   }
   cl.insert( data );

   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName } );
   try
   {
      //设置optstartcostlimit为0,不通过代价计算选择查询计划
      db.updateConf( { optstartcostlimit: 0 } );
      var indexName = cl.find().sort( { "a": 1 } ).limit( 10 ).skip( 1 ).explain().current().toObj()["IndexName"];
      assert.equal( indexName, "" );

      //设置optstartcostlimit为最大值2^31-1,通过代价计算选择查询计划
      db.updateConf( { optstartcostlimit: 2147483647 } );
      var indexName = cl.find().sort( { "a": 1 } ).limit( 10 ).skip( 1 ).explain().current().toObj()["IndexName"];
      assert.equal( indexName, "a" );

      //设置optstartcostlimit为非法值
      // 指定非数值类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( { optstartcostlimit: "100" } );
      } );

      // 指定超出范围的数值：-1、2^31（自动修正为最大值）
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( { optstartcostlimit: -1 } );
      } );
   }
   finally
   {
      db.deleteConf( { optstartcostlimit: 1 } );
   }
}

