/******************************************************************************
 * @Description   : seqDB-32191:bool类型调用$format
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.14
 * @LastEditTime  : 2023.06.14
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32191";

main( test )
function test ( testPara )
{
   // 集合插入数据
   var cl = testPara.testCL;
   var docs = [
      { No: 1, a: true },
      { No: 2, a: false },
   ];
   cl.insert( docs );

   // 作为选择符
   // scale > 0
   var actRecords = cl.find( {}, { "a": { "$format": 1 } } );
   var expRecords = [
      { No: 1, a: "1.0" },
      { No: 2, a: "0.0" }
   ];
   commCompareResults( actRecords, expRecords );

   // scale = 0
   var actRecords = cl.find( {}, { "a": { "$format": 0 } } );
   var expRecords = [
      { No: 1, a: "1" },
      { No: 2, a: "0" }
   ];
   commCompareResults( actRecords, expRecords );

   // scale < 0
   var actRecords = cl.find( {}, { "a": { "$format": -1 } } );
   commCompareResults( actRecords, expRecords );

   // 作为匹配符
   // scale > 0
   var actRecords = cl.find( { "a": { "$format": 2, $et: "0.00" } } );
   var expRecords = [ { No: 2, a: false } ];
   commCompareResults( actRecords, expRecords );

   // scale = 0
   var actRecords = cl.find( { "a": { "$format": 0, $et: "1" } } );
   var expRecords = [ { No: 1, a: true } ];
   commCompareResults( actRecords, expRecords );

   // scale < 0
   var actRecords = cl.find( { "a": { "$format": -2, $et: "0" } } );
   var expRecords = [ { No: 2, a: false } ];
   commCompareResults( actRecords, expRecords );
}