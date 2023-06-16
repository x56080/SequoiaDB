/******************************************************************************
 * @Description   : seqDB-32203:date类型调用$month
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32203";

main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: { $date: "2000-03-11" } },
        { No: 2, a: { $date: "0000-01-01" } },
        { No: 3, a: { $date: "9999-12-31" } },
        { No: 4, a: { $date: "2024-02-29" } },
    ];
    cl.insert( docs );

    // 作为选择符
    var actRecords = cl.find( {}, { "a": { "$month": 1 } } );
    var expRecords = [
        { No: 1, a: 3 },
        { No: 2, a: 1 },
        { No: 3, a: 12 },
        { No: 4, a: 2 },
    ];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    var actRecords = cl.find( { "a": { "$month": 1, "$et": 3 } } );
    commCompareResults( actRecords, [ { No: 1, a: { $date: "2000-03-11" } } ] );
}