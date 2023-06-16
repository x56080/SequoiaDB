/******************************************************************************
 * @Description   : seqDB-32216:int64类型调用$day
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32216";
main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: { $numberLong: "1000000000000" } },         // 2001-09-09
        { No: 2, a: { $numberLong: "0" } },                     // 1970-01-01
        { No: 3, a: { $numberLong: "-9223372036854775808" } },  // -292275055-05-17
        { No: 4, a: { $numberLong: "9223372036854775807" } },   // 292278994-08-17
        { No: 5, a: { $numberLong: "1709164800000" } }          // 2024-02-29
    ];
    cl.insert( docs );

    // 作为选择符
    var actRecords = cl.find( {}, { "a": { "$day": 1 } } );
    var expRecords = [
        { No: 1, a: 9 },
        { No: 2, a: 1 },
        { No: 3, a: 17 },
        { No: 4, a: 17 },
        { No: 5, a: 29 }
    ];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    var actRecords = cl.find( { "a": { "$day": 1, "$et": 29 } } );
    commCompareResults( actRecords, [ { No: 5, a: 1709164800000 } ] );
}