/******************************************************************************
 * @Description   : seqDB-31640:$concat 方法二接口参数验证
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.17
 * @LastEditTime  : 2023.05.18
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31640";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   dbcl.insert( { No: 1, a: "sequoiadb" } );
   // { <字段名> : { $concat : [ pos, [ "<字符串>", ... ] ] } }
   // pos不为数值类型
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { "a": { "$concat": ["abc", ["a", "b", "c"]] } } ).toArray();
   } );

   // pos为decimal
   var actRecords = dbcl.find( {}, { "a": { "$concat": [{ "$decimal": "1.234" }, ["a", "b", "c"]] } } );
   var expRecords = [{ No: 1, a: "asequoiadbbc" }];
   commCompareResults( actRecords, expRecords );

   // pos为double
   actRecords = dbcl.find( {}, { "a": { "$concat": [1.234, ["a", "b", "c"]] } } );
   commCompareResults( actRecords, expRecords );

   // $concat:[],数组大小为0
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { "a": { "$concat": [] } } ).toArray();
   } );

   // $concat:[],数组大小为1
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { "a": { "$concat": [1] } } ).toArray();
   } );

   // $concat:[],数组大小为3
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { "a": { "$concat": [1, ["a", "b", "c"], 3] } } ).toArray();
   } );

   // $concat:[],数组大小为2，第二个元素不为字符串
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { "a": { "$concat": [1, "string"] } } ).toArray();
   } );

   // [ "<字符串>", ... ]，包含不能处理的元素
   var actRecords = dbcl.find( {}, { "a": { "$concat": [1, ["a", null, "c"]] } } );
   var expRecords = [{ No: 1, a: null }];
   commCompareResults( actRecords, expRecords );

   // 空串调用该函数
   dbcl.insert( { No: 2, a: "" } );
   actRecords = dbcl.find( {}, { "a": { "$concat": [1, ["a", "b", "c"]] } } );
   expRecords = [{ No: 1, a: "asequoiadbbc" }, { No: 2, a: "abc" }];
   commCompareResults( actRecords, expRecords );
}