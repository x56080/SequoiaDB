/******************************************************************************
 * @Description   : seqDB-31562:$leftBytes 接口参数验证
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.10
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31562";

main( test )
function test ( testPara )
{
   // 集合插入数据
   var dbcl = testPara.testCL;
   var docs = [{ a: "sequoiadb" }];
   dbcl.insert( docs );

   // pos、len覆盖字符串、null等非数值类型
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftBytes: true } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftBytes: null } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftBytes: "str" } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftBytes: [1, 2] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftBytes: [] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftBytes: [1] } } ).toArray();
   } );

   // pos、len覆盖decimal、浮点数等非整数类型
   actRecords = dbcl.find( {}, { a: { $leftBytes: 3.1 } } );
   commCompareResults( actRecords, [{ a: "seq" }] );

   actRecords = dbcl.find( {}, { a: { $leftBytes: { "$decimal": "3.11" } } } );
   commCompareResults( actRecords, [{ a: "seq" }] );

   // 集合中插入null，进行查询
   dbcl.remove();
   var docs = [{ a: null }];
   dbcl.insert( docs );
   var actRecords = dbcl.find( {}, { a: { $leftBytes: 1 } } );
   commCompareResults( actRecords, docs );
}