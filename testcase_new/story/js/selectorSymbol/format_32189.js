/******************************************************************************
 * @Description   : seqDB-32189:数值类型调用$format
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.14
 * @LastEditTime  : 2023.06.14
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32189";

main( test )
function test ( testPara )
{
   // 集合插入数据
   var cl = testPara.testCL;
   var array = new Array( 16383 + 1);
   digits = array.join( "4" );
   var docs = [
      { No: 1, a: { "$numberLong": "-1" } },
      { No: 2, a: { "$numberLong": "9223372036854775807" } },
      { No: 3, a: 99 },
      { No: 4, a: -2147483649 },
      { No: 5, a: 3.555 },
      { No: 6, a: 1.7e+308 },
      { No: 7, a: { $decimal: "100.444" } },
      { No: 8, a: { $decimal: "1." + digits } }
   ];
   cl.insert( docs );

   // 作为选择符
   // scale > 0
   var actRecords = cl.find( {}, { "a": { "$format": 1 } } );
   var expRecords1 = [
      { No: 1, a: "-1.0" },
      { No: 2, a: "9,223,372,036,854,775,807.0" },
      { No: 3, a: "99.0" },
      { No: 4, a: "-2,147,483,649.0" },
      { No: 5, a: "3.6" },
      { No: 6, a: "170,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000.0" },
      { No: 7, a: "100.4" },
      { No: 8, a: "1.4" }
   ];
   commCompareResults( actRecords, expRecords1 );

   // scale = 0
   var actRecords = cl.find( {}, { "a": { "$format": 0 } } );
   var expRecords2 = [
      { No: 1, a: "-1" },
      { No: 2, a: "9,223,372,036,854,775,807" },
      { No: 3, a: "99" },
      { No: 4, a: "-2,147,483,649" },
      { No: 5, a: "4" },
      { No: 6, a: "170,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000" },
      { No: 7, a: "100" },
      { No: 8, a: "1" }
   ];
   commCompareResults( actRecords, expRecords2 );

   // scale < 0
   var actRecords = cl.find( {}, { "a": { "$format": -1 } } );
   commCompareResults( actRecords, expRecords2 );

   // 作为匹配符
   // scale > 0
   var actRecords = cl.find( { "a": { "$format": 2, $et: "-1.00" } } );
   var expRecords3 = [ { No: 1, a: { "$numberLong": "-1" } } ];
   commCompareResults( actRecords, expRecords3 );

   // scale = 0
   var actRecords = cl.find( { "a": { "$format": 0, $et: "1" } } );
   var expRecords4 = [ { No: 8, a: { $decimal: "1." + digits } } ];
   commCompareResults( actRecords, expRecords4 );

   // scale < 0
   var actRecords = cl.find( { "a": { "$format": -2, $et: "1" } } );
   commCompareResults( actRecords, expRecords4 );

   // 与$type组合使用
   var actRecords = cl.find( {}, { "a": { "$format": 2, "$type": 2 } } );
   var expRecords = [
      { No: 1, a: "string" },
      { No: 2, a: "string" },
      { No: 3, a: "string" },
      { No: 4, a: "string" },
      { No: 5, a: "string" },
      { No: 6, a: "string" },
      { No: 7, a: "string" },
      { No: 8, a: "string" }
   ];
   commCompareResults( actRecords, expRecords );

   var actRecords = cl.find( { "a": { "$format": 2, "$type": 1, "$et": 2 } } );
   commCompareResults( actRecords, docs );
}