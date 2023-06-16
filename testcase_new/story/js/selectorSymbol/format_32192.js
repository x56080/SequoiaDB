/******************************************************************************
 * @Description   : seqDB-32192:array类型调用$format
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.14
 * @LastEditTime  : 2023.06.14
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32192";

main( test )
function test ( testPara )
{
   // 集合插入数据
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: [ 1234.56, true, { "$numberLong": "-1" }, -1001.345, "0.001" ] },
      { No: 2, a: [ false, "100000", "string", { "$timestamp": "2000-01-01-00.00.00.000000" }, { "test": "obj" } ] },
      { No: 3, a: [ { "test": 123 }, null, { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } ] },
   ];
   cl.insert( docs );

   // 作为选择符
   // scale > 0
   var actRecords = cl.find( {}, { "a": { "$format": 1 } } );
   var expRecords = [
      { No: 1, a: [ "1,234.6", "1.0", "-1.0", "-1,001.3", "0.0" ] },
      { No: 2, a: [ "0.0", "100,000.0", "0.0", null, null ] },
      { No: 3, a: [ null, null, null ] }
   ];
   commCompareResults( actRecords, expRecords );

   // scale = 0
   var actRecords = cl.find( {}, { "a": { "$format": 0 } } );
   var expRecords = [
      { No: 1, a: [ "1,235", "1", "-1", "-1,001", "0" ] },
      { No: 2, a: [ "0", "100,000", "0", null, null ] },
      { No: 3, a: [ null, null, null ] }
   ];
   commCompareResults( actRecords, expRecords );

   // scale < 0
   var actRecords = cl.find( {}, { "a": { "$format": -1 } } );
   commCompareResults( actRecords, expRecords );

   // 作为匹配符
   // scale > 0
   var actRecords = cl.find( { "a": { "$format": 2, $et: [ "1,234.56", "1.00", "-1.00", "-1,001.35", "0.00" ] } } );
   var expRecords = [ { No: 1, a: [ 1234.56, true, { "$numberLong": "-1" }, -1001.345, "0.001" ] } ];
   commCompareResults( actRecords, expRecords );

   // scale = 0
   var actRecords = cl.find( { "a": { "$format": 0, $et: [ "0", "100,000", "0", null, null ] } } );
   var expRecords = [ { No: 2, a: [ false, "100000", "string", { "$timestamp": "2000-01-01-00.00.00.000000" }, { "test": "obj" } ] } ];
   commCompareResults( actRecords, expRecords );

   // scale < 0
   var actRecords = cl.find( { "a": { "$format": -1, $et: [ null, null, null ] } } );
   var expRecords = [ { No: 3, a: [ { "test": 123 }, null, { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } ] } ];
   commCompareResults( actRecords, expRecords );
}