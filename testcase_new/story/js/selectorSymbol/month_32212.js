/******************************************************************************
 * @Description   : seqDB-32212:int32类型调用$month
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32212";
main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: 1000000000 },   // 2001-09-09
        { No: 2, a: 0 },            // 1970-01-01
        { No: 3, a: -2147483648 },  // 1901-12-14
        { No: 4, a: 2147483647 },   // 2038-01-19
        { No: 5, a: 1709164800 }    // 2024-02-29
    ];
    cl.insert( docs );

    // 作为选择符
    var actRecords = cl.find( {}, { "a": { "$month": 1 } } );
    var expRecords = [
        { No: 1, a: 9 },
        { No: 2, a: 1 },
        { No: 3, a: 12 },
        { No: 4, a: 1 },
        { No: 5, a: 2 }
    ];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    var actRecords = cl.find( { "a": { "$month": 1, "$et": 1 } } );
    commCompareResults( actRecords, [ { No: 2, a: 0 }, { No: 4, a: 2147483647 }  ] );
}