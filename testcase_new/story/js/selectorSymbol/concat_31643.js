/******************************************************************************
 * @Description   : seqDB-31643:$concat 数组为嵌套数组
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.17
 * @LastEditTime  : 2023.05.17
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31643";

main( test );
function test ( testPara )
{
    var dbcl = testPara.testCL;
    dbcl.insert( { No: 1, a: "abc" } );

    // { <字段名> : { $concat : [ pos, [ "<字符串>", ... ] ] } }
    // 拼接嵌套数组
    var actRecords = dbcl.find( {}, { a: { "$concat": [ 1, [ "a", [ "b", "c" ], "d" ] ] } } );
    var expRecords = [{ No: 1, a: "aabc[ \"b\", \"c\" ]d" }];
    commCompareResults( actRecords, expRecords );

    // 拼接嵌套数组，数组中包含不能处理的元素
    actRecords = dbcl.find( {}, { a: { "$concat": [ 1, [ "a", [ "b", null ], "d" ] ] } } );
    expRecords = [{ No: 1, a: "aabc[ \"b\", null ]d" }];
    commCompareResults( actRecords, expRecords );

    // 拼接嵌套数组，数组中包含空数组
    actRecords = dbcl.find( {}, { a: { "$concat": [ 1, [ "a", [ ], "d" ] ] } } );
    expRecords = [{ No: 1, a: "aabc[]d" }];
    commCompareResults( actRecords, expRecords );

    // 拼接多层嵌套数组
    actRecords = dbcl.find( {}, { a: { "$concat": [ 1, [ "a", [ "b", [ "c", "d" ], "e" ], "f" ] ] } } );
    expRecords = [{ No: 1, a: "aabc[ \"b\", [ \"c\", \"d\" ], \"e\" ]f" }];
    commCompareResults( actRecords, expRecords );

    // 作为匹配符
    actRecords = dbcl.find( { a: { "$concat": [ 1, [ "a", [ "b", "c" ], "d" ] ], $et: "aabc[ \"b\", \"c\" ]d" } } );
    expRecords = [ { No: 1, a: "abc" } ];
    commCompareResults( actRecords, expRecords );
}