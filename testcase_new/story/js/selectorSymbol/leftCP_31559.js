/******************************************************************************
 * @Description   : seqDB-31559:$leftCP 接口参数验证
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.10
 * @LastEditTime  : 2023.05.10
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31559";

main( test )
function test ( testPara )
{
   var dbcl = testPara.testCL;

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftCP: true } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftCP: null } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftCP: "str" } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftCP: [1, 2] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftCP: [] } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.find( {}, { a: { $leftCP: [1] } } ).toArray();
   } );

   var docs = [{ a: "sequoiadb" }];
   dbcl.insert( docs );
   actRecords = dbcl.find( {}, { a: { $leftCP: 3.1 } } );
   commCompareResults( actRecords, [{ a: "seq" }] );

   actRecords = dbcl.find( {}, { a: { $leftCP: { "$decimal": "3.11" } } } );
   commCompareResults( actRecords, [{ a: "seq" }] );

   dbcl.remove();
   var docs = [{ a: null }];
   dbcl.insert( docs );
   var actRecords = dbcl.find( {}, { a: { $leftCP: 1 } } );
   commCompareResults( actRecords, docs );
}