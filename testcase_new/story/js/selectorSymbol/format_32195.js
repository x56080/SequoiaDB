/******************************************************************************
 * @Description   : seqDB-32195:验证千分位分隔符是否生效
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.14
 * @LastEditTime  : 2023.06.14
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32195";

main( test )
function test ( testPara )
{
   // 集合插入数据
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: 1 },
      { No: 2, a: 12 },
      { No: 3, a: 123 },
      { No: 4, a: 1234 },
      { No: 5, a: 12345 },
      { No: 6, a: 123456 },
      { No: 7, a: 1234567 },
      { No: 8, a: 12345678 },
      { No: 9, a: 123456789 },
      { No: 10, a: 1234567890 }
   ];
   cl.insert( docs );

   // 作为选择符
   // scale > 0
   var actRecords = cl.find( {}, { "a": { "$format": 1 } } );
   var expRecords = [
      { No: 1, a: "1.0" },
      { No: 2, a: "12.0" },
      { No: 3, a: "123.0" },
      { No: 4, a: "1,234.0" },
      { No: 5, a: "12,345.0" },
      { No: 6, a: "123,456.0" },
      { No: 7, a: "1,234,567.0" },
      { No: 8, a: "12,345,678.0" },
      { No: 9, a: "123,456,789.0" },
      { No: 10, a: "1,234,567,890.0" }
   ];
   commCompareResults( actRecords, expRecords );

   // scale = 0
   var actRecords = cl.find( {}, { "a": { "$format": 0 } } );
   var expRecords = [
      { No: 1, a: "1" },
      { No: 2, a: "12" },
      { No: 3, a: "123" }, 
      { No: 4, a: "1,234" },
      { No: 5, a: "12,345" },
      { No: 6, a: "123,456" },
      { No: 7, a: "1,234,567" },
      { No: 8, a: "12,345,678" },
      { No: 9, a: "123,456,789" },
      { No: 10, a: "1,234,567,890" }
   ];
   commCompareResults( actRecords, expRecords );

   // scale < 0
   var actRecords = cl.find( {}, { "a": { "$format": -1 } } );
   commCompareResults( actRecords, expRecords );
}