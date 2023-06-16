/******************************************************************************
 * @Description   : seqDB-32210:字符串类型调用$day
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32210";
main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: "2020-10-20" },
        { No: 2, a: "0000-01-01" },
        { No: 3, a: "9999-12-31" },
        { No: 4, a: "2024-02-29" },
        { No: 5, a: "2023-02-29" },
        { No: 6, a: "2025-03-31T13:34:59" },
        { No: 7, a: "2025/05/20" }
    ];
    cl.insert( docs );

    // 作为选择符
    var actRecords = cl.find( {}, { "a": { "$day": 1 } } );
    var expRecords = [
        { No: 1, a: 20 },
        { No: 2, a: 1 },
        { No: 3, a: 31 },
        { No: 4, a: 29 },
        { No: 5, a: null },
        { No: 6, a: null },
        { No: 7, a: null }
    ];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    var actRecords = cl.find( { "a": { "$day": 1, "$et": 29 } } );
    commCompareResults( actRecords, [ { No: 4, a: "2024-02-29" } ] );

    var actRecords = cl.find( { "a": { "$day": 1, "$et": 31 } } );
    commCompareResults( actRecords, [ { No: 3, a: "9999-12-31" } ] );
}