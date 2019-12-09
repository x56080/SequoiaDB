/************************************
*@Description: find操作，匹配符$all，匹配正则和数值 
*@author:      wangkexin
*@createdate:  2019.3.5
*@testlinkCase:seqDB-11072
**************************************/

main();

function main ()
{
    var csName = COMMCSNAME;
    var clName = "cl11072";

    var cl = commCreateCL( db, csName, clName, null, null, true, false, "create cl in the begin" );

    cl.insert( { _id: 1, a: [1, { "$regex": "^W", "$options": "" }] } );
    cl.insert( { _id: 2, a: 1 } );
    cl.insert( { _id: 3, a: { "$regex": "^W", "$options": "" } } );

    var cursor = cl.find( { a: { $all: [1, { "$regex": "^W", "$options": "" }] } } );
    var expRecs = '[{"_id":1,"a":[1,{"$regex":"^W","$options":""}]}]';
    checkCLData( cursor, expRecs, 1 );

    commDropCL( db, csName, clName, true, true, "drop CL in the end" );
}

function checkCLData ( rc, expRecs, expCnt )
{
    println( "\n---Begin to check cl data." );
    var recsArray = [];
    while( tmpRecs = rc.next() )
    {
        recsArray.push( tmpRecs.toObj() );
    }
    rc.close();

    var actCnt = recsArray.length;
    var actRecs = JSON.stringify( recsArray );
    if( actCnt !== expCnt || actRecs !== expRecs )
    {
        throw buildException( "checkCLdata", null, "[find]",
            "[cnt:" + expCnt + ", recs:" + expRecs + "]",
            "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
    }
    println( "cl records: " + actRecs );
}