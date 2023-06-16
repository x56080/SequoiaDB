/******************************************************************************
 * @Description   : seqDB-32193:不能进行格式化的类型调用$format
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.14
 * @LastEditTime  : 2023.06.14
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32193";

main( test )
function test ( testPara )
{
   // 集合插入数据
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: null },
      { No: 2, a: { $date: "2000-01-01" } },
      { No: 3, a: { "$timestamp": "2000-01-01-00.00.00.000000" } },
      { No: 4, a: { "tobj": 123 } },
      { No: 5, a: { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } },
      { No: 6, a: { "$regex" : "^W", "$options" : "i" } },
      { No: 7, a: { "$decimal": "MIN" } },
      { No: 8, a: { "$decimal": "MAX" } },
      { No: 9, a: { "$decimal": "NaN" } },
      { No: 10, a: -Infinity },
      { No: 11, a: Infinity },
      { No: 12, a: NaN }
   ];
   cl.insert( docs );

   // 作为选择符
   // scale > 0
   var actRecords = cl.find( {}, { "a": { "$format": 1 } } );
   var expRecords = [
      { No: 1, a: null },
      { No: 2, a: null },
      { No: 3, a: null },
      { No: 4, a: null },
      { No: 5, a: null },
      { No: 6, a: null },
      { No: 7, a: "MIN" },
      { No: 8, a: "MAX" },
      { No: 9, a: "NaN" },
      { No: 10, a: "-inf" },
      { No: 11, a: "inf" },
      { No: 12, a: "nan" }
   ];
   commCompareResults( actRecords, expRecords );

   // scale = 0
   var actRecords = cl.find( {}, { "a": { "$format": 0 } } );
   commCompareResults( actRecords, expRecords );

   // scale < 0
   var actRecords = cl.find( {}, { "a": { "$format": -1 } } );
   commCompareResults( actRecords, expRecords );

   // 作为匹配符
   // scale > 0
   var actRecords = cl.find( { "a": { "$format": 1, $et: null } } );
   var expRecords = [ 
      { No: 1, a: null },
      { No: 2, a: { $date: "2000-01-01" } },
      { No: 3, a: { "$timestamp": "2000-01-01-00.00.00.000000" } },
      { No: 4, a: { "tobj": 123 } },
      { No: 5, a: { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } },
      { No: 6, a: { "$regex" : "^W", "$options" : "i" } }
   ];
   commCompareResults( actRecords, expRecords );

   // scale = 0
   var actRecords = cl.find( { "a": { "$format": 0, $et: null } } );
   commCompareResults( actRecords, expRecords );

   // scale < 0
   var actRecords = cl.find( { "a": { "$format": -1, $et: null } } );
   commCompareResults( actRecords, expRecords );
}