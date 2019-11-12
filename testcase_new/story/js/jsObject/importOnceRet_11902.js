/******************************************************************************
*@Description : test import js file with return and no return
*               seqDB-11902:使用import、importOnce导入js文件（无返回值）
*               seqDB-11903:使用import importOnce导入js文件（有返回值）
*@author      : Liang XueWang 
******************************************************************************/
// create js file without ret and with ret
createWithoutRetFile() ;
createWithRetFile() ;

// importOnce js file without ret
var ret = importOnce( withoutRetFile ) ;
if( typeof( ret ) !== "undefined" )
{
    throw buildException( null, null, "import js file with no ret",
                          "undefined", typeof(ret) ) ;
}
var sum = add( 1, 2 ) ;
if( sum !== 3 )
{
    throw buildException( null, null, "test use func", 3, sum ) ;
}

// importOnce js file with ret
var ret = importOnce( withRetFile ) ;
if( ret !== 6 )
{
    throw buildException( null, null, "import js file with ret, check ret",
                          6, ret ) ;
}
if( tmp !== 100 )
{
    throw buildException( null, null, "import js file with ret, check variable",
                          100, tmp ) ;
}
var pro = mul( 10, 20 ) ;
if( pro !== 200 )
{
    throw buildException( null, null, "import js file with ret, check func",
                          200, pro ) ;
}

// remove js file without ret and with ret    
removeFile( withoutRetFile ) ;
removeFile( withRetFile ) ;
