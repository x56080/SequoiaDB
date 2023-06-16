/******************************************************************************
 * @Description   : seqDB-32200:$month函数参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32200";

main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: { $date: "2000-01-01" } },
        { No: 2, a: "2012-01-01" },
        { No: 3, a: { $timestamp: "2012-01-01-13.14.26.124233" } },
        { No: 4, a: "2012/01/01" }
    ];
    cl.insert( docs );

    assert.tryThrow( SDB_INVALIDARG, function()
    { 
        cl.find( {}, { "a": { "$month": 2 } } ).toArray();
    } );

    assert.tryThrow( SDB_INVALIDARG, function()
    { 
        cl.find( {}, { "a": { "$month": true } } ).toArray();
    } );

    var actRecords = cl.find( {}, { "a": { "$month": 1 } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: 1 },
        { No: 3, a: 1 },
        { No: 4, a: null }
    ];
    commCompareResults( actRecords, expRecords );

    // 参数覆盖decimal、浮点数等非整数类型
    var actRecords = cl.find( {}, { "a": { "$month": { "$decimal": "1.1" } } } );
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$month": 1.1 } } );
    commCompareResults( actRecords, expRecords );
}