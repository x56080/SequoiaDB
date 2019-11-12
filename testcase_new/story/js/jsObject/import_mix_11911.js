/*******************************************************************
*@Description : test mix import importOnce js file
*               seqDB-11911:混合使用import importOnce导入文件
*@author      : Liang XueWang 
*******************************************************************/
// js file to mix, first import then importOnce
var mixFile1             = WORKDIR + "/mix1_11911.js" ;   
// js file to mix, first importOnce then import
var mixFile2             = WORKDIR + "/mix2_11911.js" ;   

function createMixFile()
{
    try
    {
        var file = new File( mixFile1 ) ;
        file.write( "a++;" ) ;
        file.close() ;
        file = new File( mixFile2 ) ;
        file.write( "b++;" ) ;
        file.close() ;
    }
    catch( e )
    {
        throw buildException( "createMixFile", null, "create mix file " +
              mixFile1 + " " + mixFile2, 0, e ) ;
    } 
}

// create mix use import importOnce file
createMixFile() ;

// test mix use import and importOnce
// first import then importOnce
var a = 0 ;
import( mixFile1 ) ;
importOnce( mixFile1 ) ;
if( a !== 1 )
{
    throw buildException( null, null, 
          "first import then importOnce, a value", 1, a ) ;
}

// test mix use import and importOnce
// first importOnce then import    
var b = 0 ;
importOnce( mixFile2 ) ;
import( mixFile2 ) ;
if( b !== 2 )
{
    throw buildException( null, null,
          "firt importOnce then import, b value", 2, b ) ;
}

// remove mix use import importOnce file
removeFile( mixFile1 ) ;
removeFile( mixFile2 ) ;