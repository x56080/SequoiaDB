/******************************************************************************
 * @Description   : seqDB-31496:$round严格模式下，验证边界值
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.08
 * @LastEditTime  : 2023.05.09
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31496";

main( test )
function test ( testPara )
{
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: 2147483647 },
      { No: 2, a: -2147483648 }
   ];
   cl.insert( docs );
   cl.alter( { StrictDataMode: true } );

   assert.tryThrow( SDB_VALUE_OVERFLOW, function()
   {
      cl.find( {}, { "a": { "$round": -1 } } ).toArray();
   } );
   var actRecords = cl.find( {}, { "a": { "$round": -9 } } );
   expRecords = [
      { No: 1, a: 2000000000 },
      { No: 2, a: -2000000000 }
   ];
   commCompareResults( actRecords, expRecords );

   cl.remove();
   var docs = [
      { No: 1, a: { "$numberLong": "9223372036854775807" } },
      { No: 2, a: { "$numberLong": "-9223372036854775808" } }
   ];
   cl.insert( docs );

   assert.tryThrow( SDB_VALUE_OVERFLOW, function()
   {
      cl.find( {}, { "a": { "$round": -19 } } ).toArray();
   } );

   var actRecords = cl.find( {}, { "a": { "$round": -18 } } );
   expRecords = [
      { No: 1, a: { "$numberLong": "9000000000000000000" } },
      { No: 2, a: { "$numberLong": "-9000000000000000000" } }
   ];
   commCompareResults( actRecords, expRecords );

   cl.remove();
   var docs = [
      { No: 1, a: -1.7E+308 },
      { No: 2, a: 1.7e+308 }
   ];
   cl.insert( docs );

   assert.tryThrow( SDB_VALUE_OVERFLOW, function()
   {
      cl.find( {}, { "a": { "$round": -308 } } ).toArray();
   } );

   var actRecords = cl.find( {}, { "a": { "$round": -307 } } );
   expRecords = [
      { No: 1, a: -1.7e+308 },
      { No: 2, a: 1.7e+308 }
   ];
   commCompareResults( actRecords, expRecords );
}