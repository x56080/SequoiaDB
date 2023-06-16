/******************************************************************************
 * @Description   : seqDB-32188:$format函数参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.14
 * @LastEditTime  : 2023.06.14
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32188";

main( test )
function test ( testPara )
{
   // 集合插入数据
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: true },
      { No: 2, a: -13.14 },
      { No: 3, a: 300 },
      { No: 4, a: "三十" },
   ];
   cl.insert( docs );

   // 作为选择符
   // scale覆盖字符串、null、date、timestamp、array、undefined等非数值类型
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$format": "str" } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$format": null } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$format": { $date: "2000-01-01" } } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$format": { "$timestamp": "2012-01-01-13.14.26.124233" } } } ).toArray();
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$format": [1, 2, 3] } } ).toArray();
   } );

   // scale覆盖decimal、浮点数等非整数类型
   var actRecords = cl.find( {}, { "a": { "$format": { "$decimal": "1.1" } } } );
   var expRecords1 = [
      { No: 1, a: "1.0" },
      { No: 2, a: "-13.1" },
      { No: 3, a: "300.0" },
      { No: 4, a: "0.0" }
   ];
   commCompareResults( actRecords, expRecords1 );

   var actRecords = cl.find( {}, { "a": { "$format": 1.7 } } );
   commCompareResults( actRecords, expRecords1 );

   // scale超过规定范围
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( {}, { "a": { "$format": 16384 } } ).toArray();
   } );

   // scale为正常范围的整数值
   var actRecords = cl.find( { No: 1 }, { "a": { "$format": 16383 } } );
   var array = new Array( 16383 + 1);
   array = array.join( "0" );
   var expRecords2 = [ { No: 1, a: "1." + array } ];
   commCompareResults( actRecords, expRecords2 );
}