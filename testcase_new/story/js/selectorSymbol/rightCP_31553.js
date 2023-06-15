/******************************************************************************
 * @Description   : seqDB-31553:$rightCP 接口参数验证
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.10
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31553";

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
      dbcl.find( {}, { a: { $rightCP: true } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $rightCP: null } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $rightCP: "str" } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $rightCP: [1, 2] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $rightCP: [] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $rightCP: [1] } } ).toArray();
   } );

   // pos、len覆盖decimal、浮点数等非整数类型
   actRecords = dbcl.find( {}, { a: { $rightCP: 3.1 } } );
   commCompareResults( actRecords, [{ a: "adb" }] );

   actRecords = dbcl.find( {}, { a: { $rightCP: { "$decimal": "3.11" } } } );
   commCompareResults( actRecords, [{ a: "adb" }] );

   // 集合中插入null，进行查询
   dbcl.remove();
   var docs = [{ a: null }];
   dbcl.insert( docs );
   var actRecords = dbcl.find( {}, { a: { $rightCP: 1 } } );
   commCompareResults( actRecords, docs );
}