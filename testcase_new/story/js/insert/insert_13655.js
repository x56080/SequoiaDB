/************************************
*@Description: 插入记录失败后，重新执行查询操作从上一次的数据节点查询
*@author:      wangkexin
*@createDate:  2019.3.12
*@testlinkCase: seqDB-13655
**************************************/
main();
function main ()
{
    var clName = "cl13655";

    //插入正常记录，执行查询，检查结果
    var cl = commCreateCL( db, COMMCSNAME, clName );
    var insert_string = { _id: 1, field: 123, name: "赵钱孙李陈" };
    cl.insert( insert_string );
    cl.createIndex( "myIndex13655", { field: 1, name: 1 } );
    checkRecords( cl, insert_string );
    var expNodeName = getNodeName( cl );

    //插入部分失败记录，返回错误码覆盖:-6,-24,-37,-38,-39,-108
    var errInsertString1 = { _id: [1, 2, 3] };
    checkErrCodeAndNodeName( cl, errInsertString1, -6, expNodeName );

    var errInsertString2 = { a: repeat( "a", 16 * 1024 * 1024 ) };
    checkErrCodeAndNodeName( cl, errInsertString2, -24, expNodeName );

    var errInsertString3 = { field: [1, 2, 3], name: [4, 5, 6] };
    checkErrCodeAndNodeName( cl, errInsertString3, -37, expNodeName );

    var errInsertString4 = { _id: 1 };
    checkErrCodeAndNodeName( cl, errInsertString4, -38, expNodeName );

    var errInsertString5 = { field: repeat( "field", 1024 ) };
    checkErrCodeAndNodeName( cl, errInsertString5, -39, expNodeName );

    //清除环境
    commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}

function checkRecords ( cl, insert_string )
{
    if( cl.count() != 1 )
        throw "number error";
    robj = cl.find().next().toObj();
    if( !compareObj( insert_string, robj ) )
        throw "compare failed";
}

function checkErrCodeAndNodeName ( cl, errInsString, errorCode, expNodeName )
{
    try
    {
        cl.insert( errInsString );
        throw "expect fail but success.";
    }
    catch( e )
    {
        if( e !== errorCode )
        {
            throw buildException( "checkErrCodeAndNodeName()", e, "insert record", errorCode, e );
        }
    }

    var actNodeName = getNodeName( cl );
    if( !compareObj( expNodeName, actNodeName ) )
    {
        throw buildException( "checkErrCodeAndNodeName()", null, "compare node name after test error code = " + errorCode, expNodeName, actNodeName );
    }
}

function getNodeName ( cl )
{
    var exp = cl.find().explain().toArray();
    var obj = eval( '(' + exp + ')' );
    var NodeName = obj['NodeName'];
    return NodeName;
}

function repeat ( str, n )
{
    return new Array( n + 1 ).join( str );
}