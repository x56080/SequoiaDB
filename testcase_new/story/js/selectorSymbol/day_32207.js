/******************************************************************************
 * @Description   : seqDB-32207:timestamp类型调用$day
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32207";

main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: { "$timestamp": "2012-01-01-13.14.26.124233" } },
        { No: 2, a: { "$timestamp": "1902-01-01-00.00.00.000000" } },
        { No: 3, a: { "$timestamp": "2037-12-31-23.59.59.999999" } },
        { No: 4, a: { "$timestamp": "2024-02-29-00.00.00.000000" } }
    ];
    cl.insert( docs );

    // 作为选择符
    var actRecords = cl.find( {}, { "a": { "$day": 1 } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: 1 },
        { No: 3, a: 31 },
        { No: 4, a: 29 }
    ];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    var actRecords = cl.find( { "a": { "$day": 1, "$et": 29 } } );
    commCompareResults( actRecords, [ { No: 4, a: { "$timestamp": "2024-02-29-00.00.00.000000" } } ] );
}