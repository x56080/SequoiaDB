/******************************************************************************
*@Description: 插入记录，指定oid值非法
*@author:      liuxiaoxuan
*@createdate:  2019.10.10
*@testlinkCase: seqDB-19949:插入记录，指定oid值非法
******************************************************************************/

main();
function main()
{
    var clName = COMMCLNAME + "_insert19949";	
    commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the beginning" ) ; 	
    var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
    //插入Oid值长度小于24字节
    try
    {
        dbcl.insert( {a : {"$oid" : "123abcd00af12358902300"} } );   
    }
    catch ( e )
    {
        if( -6 !== e )
        {
            throw new Error( "insert fail: " + getErr(e) );
        }			
    }
	
    //插入Oid值长度大于24字节
    try
    {
        dbcl.insert( {a : {"$oid" : "123abcd00af12358902300123456"} } );   
    }
    catch ( e )
    {
        if( -6 !== e )
        {
            throw new Error( "insert fail: " + getErr(e) );
        }			
    }
	
    //插入Oid值长度等于24字节但内容不正确
    try
    {
        dbcl.insert( {a : ObjectId( "123abcd00ef12358902300eg") } );   
    }
    catch ( e )
    {
        if( -6 !== e )
        {
            throw new Error( "insert fail: " + getErr(e) );
        }			
    }

    commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the end" ) ;
}

