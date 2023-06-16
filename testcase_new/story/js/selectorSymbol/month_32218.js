/******************************************************************************
 * @Description   : seqDB-32218:数组类型调用$month
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.16
 * @LastEditTime  : 2023.06.16
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32218";
main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: [ 1, 2, 3 ] },
        { No: 2, a: [ "2022-2-22", { $date: "2022-2-22"}, { $timestamp: "2022-02-22-13.14.26.124233" } ] },
        { No: 3, a: [ "2025/05/20", 1709164800, 1709164800000.001 ] },
        { No: 4, a: [ { $date: "2022-2-22"}, 0, "" ] },
        { No: 5, a: [ { $decimal: "9223372036854775807.1" }, { "obj": "2021-01-30" } ] },
        { No: 6, a: [ true, null, undefined ] }
    ];
    cl.insert( docs );

    // 作为选择符
    var actRecords = cl.find( {}, { "a": { "$month": 1 } } );
    var expRecords = [
        { No: 1, a: [ 1, 1, 1 ] },
        { No: 2, a: [ 2, 2, 2 ] },
        { No: 3, a: [ null, 2, 2 ] },
        { No: 4, a: [ 2, 1, null ] },
        { No: 5, a: [ null, null ] },
        { No: 6, a: [ null, null ] }
    ];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    var actRecords = cl.find( { "a": { "$month": 1, "$et": [ null, null ] } } );
    var expRecords = [
        { No: 5, a: [ { $decimal: "9223372036854775807.1" }, { "obj": "2021-01-30" } ] },
        { No: 6, a: [ true, null ] }
    ];
    commCompareResults( actRecords, expRecords );
}