/***********************************************************************
*@Description : test import importOnce js file with postfix not .js
*               seqDB-12215:使用import importOnce导入后缀名不为.js的js文件
*@author      : Liang XueWang 
***********************************************************************/
// js file to import importOnce with postfix not .js
var importPostfixFile = WORKDIR + "/importPostfix_12215.txt";
var importOncePostfixFile = WORKDIR + "/importOncePostfix_12215.csv";

function createPostfixFile ()
{
    try
    {
        var file = new File( importPostfixFile );
        file.write( "a++" );
        file.close();
        file = new File( importOncePostfixFile );
        file.write( "b++" );
        file.close();
    }
    catch( e )
    {
        throw buildException( "createPostfixFile", e,
            "create postfix file " + importPostfixFile + " " + importOncePostfixFile,
            0, e );
    }
}

// create file
createPostfixFile();

// import importOnce file
var a = 0;
var b = 0;
import( importPostfixFile );
importOnce( importOncePostfixFile );

if( a !== 1 || b !== 1 )
{
    throw buildException( null, null, "check variable after import importOnce",
        "1 1", a + " " + b );
}

// remove file
removeFile( importPostfixFile );
removeFile( importOncePostfixFile );
