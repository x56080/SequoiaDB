/******************************************************************************
 * @Description   : seqDB-31370:$round int调用验证
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.08
 * @LastEditTime  : 2023.05.08
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31370";

main( test )
function test ( testPara )
{
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: -9 }, { No: 2, a: -8 }, { No: 3, a: -7 }, { No: 4, a: -6 }, 
        { No: 5, a: -5 }, { No: 6, a: -4 }, { No: 7, a: -3 }, { No: 8, a: -2 },
        { No: 9, a: -1 }, { No: 10, a: 0 }, { No: 11, a: 1 }, { No: 12, a: 2 },
        { No: 13, a: 3 }, { No: 14, a: 4 }, { No: 15, a: 5 }, { No: 16, a: 6 },
        { No: 17, a: 7 }, { No: 18, a: 8 }, { No: 19, a: 9 }
    ];
    cl.insert( docs );

    var actRecords = cl.find( {}, { "a": { "$round": -1 } } );
    var expRecords = [
        { No: 1, a: -10 }, { No: 2, a: -10 }, { No: 3, a: -10 }, { No: 4, a: -10 }, 
        { No: 5, a: -10 }, { No: 6, a: 0 }, { No: 7, a: 0 }, { No: 8, a: 0 },
        { No: 9, a: 0 }, { No: 10, a: 0 }, { No: 11, a: 0 }, { No: 12, a: 0 },
        { No: 13, a: 0 }, { No: 14, a: 0 }, { No: 15, a: 10 }, { No: 16, a: 10 },
        { No: 17, a: 10 }, { No: 18, a: 10 }, { No: 19, a: 10 }
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( {}, { "a": { "$round": 0 } } );
    var expRecords = [
        { No: 1, a: -9 }, { No: 2, a: -8 }, { No: 3, a: -7 }, { No: 4, a: -6 }, 
        { No: 5, a: -5 }, { No: 6, a: -4 }, { No: 7, a: -3 }, { No: 8, a: -2 },
        { No: 9, a: -1 }, { No: 10, a: 0 }, { No: 11, a: 1 }, { No: 12, a: 2 },
        { No: 13, a: 3 }, { No: 14, a: 4 }, { No: 15, a: 5 }, { No: 16, a: 6 },
        { No: 17, a: 7 }, { No: 18, a: 8 }, { No: 19, a: 9 }
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( {}, { "a": { "$round": 1 } } );
    expRecords = [
        { No: 1, a: -9 }, { No: 2, a: -8 }, { No: 3, a: -7 }, { No: 4, a: -6 }, 
        { No: 5, a: -5 }, { No: 6, a: -4 }, { No: 7, a: -3 }, { No: 8, a: -2 },
        { No: 9, a: -1 }, { No: 10, a: 0 }, { No: 11, a: 1 }, { No: 12, a: 2 },
        { No: 13, a: 3 }, { No: 14, a: 4 }, { No: 15, a: 5 }, { No: 16, a: 6 },
        { No: 17, a: 7 }, { No: 18, a: 8 }, { No: 19, a: 9 }
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( { "a": { "$round": -1, "$et": -10 } } ) ;
    expRecords = [
        { No: 1, a: -9 }, { No: 2, a: -8 }, { No: 3, a: -7 }, 
        { No: 4, a: -6 }, { No: 5, a: -5 }
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( { "a": { "$round": -1, "$et": 0 } } ) ;
    expRecords = [
        { No: 6, a: -4 }, { No: 7, a: -3 }, { No: 8, a: -2 },{ No: 9, a: -1 }, 
        { No: 10, a: 0 }, { No: 11, a: 1 }, { No: 12, a: 2 },{ No: 13, a: 3 }, 
        { No: 14, a: 4 }
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( { "a": { "$round": -1, "$et": 10 } } ) ;
    expRecords = [
        { No: 15, a: 5 }, { No: 16, a: 6 }, { No: 17, a: 7 }, { No: 18, a: 8 }, 
        { No: 19, a: 9 }
    ];
    commCompareResults( actRecords, expRecords );
}