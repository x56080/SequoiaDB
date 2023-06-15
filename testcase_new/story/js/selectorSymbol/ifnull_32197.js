/******************************************************************************
 * @Description   : seqDB-32197:视为null的类型调用$ifnull
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32197";

main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: 1 },
        { No: 2, a: null },
        { No: 3, a: undefined },
    ];  
    cl.insert( docs );

    // 作为选择符
    // {a:{$ifnull:value}}
    var actRecords = cl.find( {}, { "a": { "$ifnull": 1 } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: 1 },
        { No: 3, a: 1 },
    ];
    commCompareResults( actRecords, expRecords );

    // {a:{$ifnull:value,$type:1}}
    var actRecords = cl.find( {}, { "a": { "$ifnull": "str", "$type": 1 } } );
    var expRecords = [
        { No: 1, a: 16 },
        { No: 2, a: 2 },
        { No: 3, a: 2 }
    ];
    commCompareResults( actRecords, expRecords );

    // {a:{$ifnull:value,$type:2}}
    var actRecords = cl.find( {}, { "a": { "$ifnull": "str", "$type": 2 } } );
    var expRecords = [
        { No: 1, a: "int32" },
        { No: 2, a: "string" },
        { No: 3, a: "string" }
    ];
    commCompareResults( actRecords, expRecords );

    // {b:{$ifnull:value}}
    var actRecords = cl.find( {}, { "b": { "$ifnull": 1.1 } } );
    var expRecords = [
        { No: 1, a: 1, b: 1.1 },
        { No: 2, a: null, b: 1.1 },
        { No: 3, b: 1.1 }
    ];
    commCompareResults( actRecords, expRecords );


    // 作为匹配符
    // {a:{$ifnull:value，$et:value}}
    var actRecords = cl.find( { "a": { "$ifnull": true, "$et": true } } );
    var expRecords = [
        { No: 2, a: null },
        { No: 3 }
    ];
    commCompareResults( actRecords, expRecords );

    // {a:{$ifnull:value，$type:1, $et:value类型的数值}}
    var actRecords = cl.find( { "a": { "$ifnull": { "$date": "2000-01-01" }, "$type": 1, "$et": 9 } } );
    var expRecords = [
        { No: 2, a: null },
        { No: 3 }
    ];
    commCompareResults( actRecords, expRecords );

    // {a:{$ifnull:value，$type:2, $et:value类型的字符串}}
    var actRecords = cl.find( { "a": { "$ifnull": { "$timestamp": "2000-01-01-00.00.00.000000" }, "$type": 2, "$et": "timestamp" } } );
    var expRecords = [
        { No: 2, a: null },
        { No: 3 }
    ];
    commCompareResults( actRecords, expRecords );

    // {b:{$ifnull:value，$et:value}}
    var actRecords = cl.find( { "b": { "$ifnull": [ 1, 2, 3 ], "$et": [ 1, 2, 3 ] } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: null },
        { No: 3 }
    ];
    commCompareResults( actRecords, expRecords );

    // {a:{$isnull:1}}
    var actRecords = cl.find( { "a": { "$isnull": 1 } } );
    var expRecords = [
        { No: 2, a: null },
        { No: 3 }
    ];
    commCompareResults( actRecords, expRecords );

    // {a:{ $ifnull:value, $isnull:0}}
    var actRecords = cl.find( { "a": { "$ifnull": 1, "$isnull": 1 } } );
    var expRecords = [];
    commCompareResults( actRecords, expRecords );
}