/******************************************************************************
 * @Description   : seqDB-32233:不可转换为日期的类型调用$day
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.16
 * @LastEditTime  : 2023.06.16
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32233";
main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a:"" },
        { No: 2, a: null },
        { No: 3, a: { "$oid" : "5d1eea4d7e9eb6328c0c463e" } }, 
        { No: 4, a: false },
        { No: 5, a: { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } },
        { No: 6, a: { "$regex" : "^W", "$options" : "i" } },
        { No: 7, a: { "obj" : "2012-02-22" } },
    ];
    cl.insert( docs );

    // 作为选择符
    var actRecords = cl.find( {}, { "a": { "$day": 1 } } );
    var expRecords = [
        { No: 1, a: null },
        { No: 2, a: null },
        { No: 3, a: null },
        { No: 4, a: null },
        { No: 5, a: null },
        { No: 6, a: null },
        { No: 7, a: null }
    ];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    var actRecords = cl.find( { "a": { "$day": 1, "$et": null } } );
    commCompareResults( actRecords, docs );
}