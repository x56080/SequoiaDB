/******************************************************************************
 * @Description   : seqDB-32190:字符串类型调用$format
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.14
 * @LastEditTime  : 2023.06.14
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32190";

main( test )
function test ( testPara )
{
   // 集合插入数据
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: "123,456" },
      { No: 2, a: "2012-01-01" },
      { No: 3, a: "" },
      { No: 4, a: "123456.789" },
      { No: 5, a: "1.7e2" },
   ];
   cl.insert( docs );

   // 作为选择符
   // scale > 0
   var actRecords = cl.find( {}, { "a": { "$format": 1 } } );
   var expRecords = [
      { No: 1, a: "123.0" },
      { No: 2, a: "2,012.0" },
      { No: 3, a: "0.0" },
      { No: 4, a: "123,456.8" },
      { No: 5, a: "170.0" }
   ];
   commCompareResults( actRecords, expRecords );

   // scale = 0
   var actRecords = cl.find( {}, { "a": { "$format": 0 } } );
   var expRecords = [
      { No: 1, a: "123" },
      { No: 2, a: "2,012" },
      { No: 3, a: "0" },
      { No: 4, a: "123,457" },
      { No: 5, a: "170" }
   ];
   commCompareResults( actRecords, expRecords );

   // scale < 0
   var actRecords = cl.find( {}, { "a": { "$format": -1 } } );
   commCompareResults( actRecords, expRecords );

   // 作为匹配符
   // scale > 0
   var actRecords = cl.find( { "a": { "$format": 2, $et: "0.00" } } );
   var expRecords = [ { No: 3, a: "" } ];
   commCompareResults( actRecords, expRecords );

   // scale = 0
   var actRecords = cl.find( { "a": { "$format": 0, $et: "123,457" } } );
   var expRecords = [ { No: 4, a: "123456.789" } ];
   commCompareResults( actRecords, expRecords );

   // scale < 0
   var actRecords = cl.find( { "a": { "$format": -2, $et: "170" } } );
   var expRecords = [ { No: 5, a: "1.7e2" } ];
   commCompareResults( actRecords, expRecords );
}