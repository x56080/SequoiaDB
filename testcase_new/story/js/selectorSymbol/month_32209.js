/******************************************************************************
 * @Description   : seqDB-32209:字符串类型调用$month
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32209";
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
    var actRecords = cl.find( {}, { "a": { "$month": 1 } } );
    var expRecords = [
        { No: 1, a: 10 },
        { No: 2, a: 1 },
        { No: 3, a: 12 },
        { No: 4, a: 2 },
        { No: 5, a: null },
        { No: 6, a: null },
        { No: 7, a: null }
    ];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    var actRecords = cl.find( { "a": { "$month": 1, "$et": 12 } } );
    commCompareResults( actRecords, [ { No: 3, a: "9999-12-31" } ] );

    var actRecords = cl.find( { "a": { "$month": 1, "$et": 3 } } );
    commCompareResults( actRecords, [] );
}