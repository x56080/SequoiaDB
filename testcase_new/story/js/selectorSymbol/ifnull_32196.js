/******************************************************************************
 * @Description   : seqDB-32196:$ifnull函数参数校验
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.06.15
 * @LastEditTime  : 2023.06.15
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "32196";

main( test )
function test ( testPara )
{
    // 集合插入数据
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: 1 },
        { No: 2, a: null },
        { No: 3, a: "abc" },
        { No: 4, a: undefined }
    ];  
    cl.insert( docs );

    // 参数类型覆盖：NumberInt、NumberLong、decimal、float、string、oid、bool、date、
    // timestamp、binary、regex、array、object、null、minkey、maxkey、""、undefined
    var actRecords = cl.find( {}, { "a": { "$ifnull": 1 } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: 1 },
        { No: 3, a: "abc" },
        { No: 4, a: 1 }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "$numberLong": "9223372036854775807" } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: 9223372036854775807 },
        { No: 3, a: "abc" },
        { No: 4, a: 9223372036854775807 }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": 1.1 } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: 1.1 },
        { No: 3, a: "abc" },
        { No: 4, a: 1.1 }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "$decimal": "1.1" } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: { "$decimal": "1.1" } },
        { No: 3, a: "abc" },
        { No: 4, a: { "$decimal": "1.1" } }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": "str" } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: "str" },
        { No: 3, a: "abc" },
        { No: 4, a: "str" }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "$oid": "5f0b5a7c0000000000000000" } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: { "$oid": "5f0b5a7c0000000000000000" } },
        { No: 3, a: "abc" },
        { No: 4, a: { "$oid": "5f0b5a7c0000000000000000" } }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": true } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: true },
        { No: 3, a: "abc" },
        { No: 4, a: true }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "$date": "2000-01-01" } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: { "$date": "2000-01-01" } },
        { No: 3, a: "abc" },
        { No: 4, a: { "$date": "2000-01-01" } }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "$timestamp": "2000-01-01-00.00.00.000000" } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: { "$timestamp": "2000-01-01-00.00.00.000000" } },
        { No: 3, a: "abc" },
        { No: 4, a: { "$timestamp": "2000-01-01-00.00.00.000000" } }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } },
        { No: 3, a: "abc" },
        { No: 4, a: { "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" } }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "$regex" : "^adb", "$options" : "i" } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: { "$regex" : "^adb", "$options" : "i" } },
        { No: 3, a: "abc" },
        { No: 4, a: { "$regex" : "^adb", "$options" : "i" } }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": [ 1, 2, 3 ] } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: [ 1, 2, 3 ] },
        { No: 3, a: "abc" },
        { No: 4, a: [ 1, 2, 3 ] }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "test": "obj" } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: { "test": "obj" } },
        { No: 3, a: "abc" },
        { No: 4, a: { "test": "obj" } }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": null } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: null },
        { No: 3, a: "abc" },
        { No: 4, a: null }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "$minKey": 1 } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: { "$minKey": 1 } },
        { No: 3, a: "abc" },
        { No: 4, a: { "$minKey": 1 } }
    ];
    commCompareResults( actRecords, expRecords );

    var actRecords = cl.find( {}, { "a": { "$ifnull": { "$maxKey": 1 } } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: { "$maxKey": 1 } },
        { No: 3, a: "abc" },
        { No: 4, a: { "$maxKey": 1 } }
    ];

    var actRecords = cl.find( {}, { "a": { "$ifnull": "" } } );
    var expRecords = [
        { No: 1, a: 1 },
        { No: 2, a: "" },
        { No: 3, a: "abc" },
        { No: 4, a: "" }
    ];
    commCompareResults( actRecords, expRecords );
}