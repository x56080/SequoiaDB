/******************************************************************************
 * @Description   : seqDB-31546:$substrCP函数参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.09
 * @LastEditTime  : 2023.05.09
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31546";

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
      dbcl.find( {}, { a: { $substrCP: true } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $substrCP: null } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $substrCP: [1, true] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $substrCP: "1" } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $substrCP: [1] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $substrCP: [null, true] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $substrCP: [1, 2, 3] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $substrCP: [1, [2, 3]] } } ).toArray();
   } );

   // pos、len覆盖decimal、浮点数等非整数类型
   actRecords = dbcl.find( {}, { a: { $substrCP: 3.1 } } );
   commCompareResults( actRecords, [{ a: "seq" }] );

   actRecords = dbcl.find( {}, { a: { $substrCP: { "$decimal": "3.11" } } } );
   commCompareResults( actRecords, [{ a: "seq" }] );

   // 集合中插入null，进行查询
   dbcl.remove();
   var docs = [{ a: null }];
   dbcl.insert( docs );
   var actRecords = dbcl.find( {}, { a: { $substrCP: 1 } } );
   commCompareResults( actRecords, docs );
}