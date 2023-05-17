/******************************************************************************
 * @Description   : seqDB-31369:$round decimal调用验证
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.05.08
 * @LastEditTime  : 2023.05.08
 * @LastEditors   : Cheng Jingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "31369";

main( test )
function test ( testPara )
{
    var cl = testPara.testCL;
    var docs = [
        { No: 1, a: { "$decimal": "-922337203685477580.8" } },
        { No: 2, a: { "$decimal": "922337203685477580.2" } },
        { No: 3, a: { "$decimal": "-922337203685477589.05" } },
        { No: 4, a: { "$decimal": "922337203685477589.04" } }
    ];
    cl.insert( docs );

    var actRecords = cl.find( {}, { "a": { "$round": -1 } } );
    var expRecords = [
        { No: 1, a: { "$decimal": "-922337203685477580" } },
        { No: 2, a: { "$decimal": "922337203685477580" } },
        { No: 3, a: { "$decimal": "-922337203685477590" } },
        { No: 4, a: { "$decimal": "922337203685477590" } }
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( {}, { "a": { "$round": 0 } } );
    expRecords = [
        { No: 1, a: { "$decimal": "-922337203685477581" } },
        { No: 2, a: { "$decimal": "922337203685477580" } },
        { No: 3, a: { "$decimal": "-922337203685477589" } },
        { No: 4, a: { "$decimal": "922337203685477589" } }
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( {}, { "a": { "$round": 1 } } );
    expRecords = [
        { No: 1, a: { "$decimal": "-922337203685477580.8" } },
        { No: 2, a: { "$decimal": "922337203685477580.2" } },
        { No: 3, a: { "$decimal": "-922337203685477589.1" } },
        { No: 4, a: { "$decimal": "922337203685477589.0" } }
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( { "a": { "$round": -1, "$et": { "$decimal": "922337203685477580" } } } ) ;
    expRecords = [
        { No: 2, a: { "$decimal": "922337203685477580.2" } },
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( { "a": { "$round": 0, "$et": { "$decimal": "922337203685477589" } } } ) ;
    expRecords = [
        { No: 4, a: { "$decimal": "922337203685477589.04" } }
    ];
    commCompareResults( actRecords, expRecords );

    actRecords = cl.find( { "a": { "$round": 1, "$et": { "$decimal": "-922337203685477589.1" } } } ) ;
    expRecords = [
        { No: 3, a: { "$decimal": "-922337203685477589.05" } }
    ];
    commCompareResults( actRecords, expRecords );
}

