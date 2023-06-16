/******************************************************************************
 * @Description   : seqDB-32230:decimal类型调用$year
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32230";
main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: { $decimal: "1000000000000" } },             // 2001-09-09
        { No: 2, a: { $decimal: "0" } },                         // 1970-01-01
        { No: 3, a: { $decimal: "-9223372036854775808" } },      // -292275055-05-17
        { No: 4, a: { $decimal: "-9223372036854775808.01" } },   // 超出范围
        { No: 5, a: { $decimal: "9223372036854775807" } },       // 292278994-08-17
        { No: 6, a: { $decimal: "9223372036854775807.1" } },     // 超出范围
        { No: 7, a: { $decimal: "1709164800000.001" } },         // 2024-02-29
    ];
    cl.insert( docs );

    // 作为选择符
    var actRecords = cl.find( {}, { "a": { "$year": 1 } } );
    var expRecords = [
        { No: 1, a: 2001 },
        { No: 2, a: 1970 },
        { No: 3, a: -292275055 },
        { No: 4, a: null },
        { No: 5, a: 292278994 },
        { No: 6, a: null },
        { No: 7, a: 2024 }
    ];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    var actRecords = cl.find( { "a": { "$year": 1, "$et": 2024 } } );
    commCompareResults( actRecords, [ { No: 7, a: { $decimal: "1709164800000.001" } } ] );
}