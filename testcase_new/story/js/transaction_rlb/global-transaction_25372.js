/******************************************************************************
 * @Description   : seqDB-25372:standalone模式下需要正确执行全局事务
 * @Author        : 钟子明
 * @CreateTime    : 2022.02.16
 * @LastEditTime  : 2022.02.16
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_globtrans25372";

main( test );

function test ( testPara )
{
   //只在独立模式下测试，非独立模式下跳过测试
   if( !commIsStandalone( db ) )
   {
      return;
   }

   db.transBegin();
   testPara.testCL.insert( { acd: 123 } );
   assert.notEqual( testPara.testCL.count(), 0, '未正确插入数据' );

   assert.equal( db.snapshot( SDB_SNAP_TRANSACTIONS_CURRENT ).toArray().length, 1, '未查询到事务操作' );

   db.transRollback();
   assert.equal( testPara.testCL.count(), 1, '未正确回滚事务' );
}