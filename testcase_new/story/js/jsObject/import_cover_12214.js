/***********************************************************************
*@Description : test import importOnce js file, check cover var and func
*               seqDB-12214:使用import importOnce导入重名变量和函数
*@author      : Liang XueWang 
***********************************************************************/
// js file to cover variable and function
var importCoverFile = WORKDIR + "/importCover_12214.js";
var importOnceCoverFile = WORKDIR + "/importOnceCover_12214.js";

// variable and function to be covered
var tmp1 = 10;
function foo1 () { return 1; }
var tmp2 = "a";
function foo2 () { return "abc"; }

function createCoverFile ()
{
    try
    {
        var file = new File( importCoverFile );
        file.write( "var tmp1 = 20; function foo1() { return 2; }" );
        file.close();
        file = new File( importOnceCoverFile );
        file.write( "var tmp2 = \"b\"; function foo2() { return \"def\"; }" );
        file.close();
    }
    catch( e )
    {
        throw buildException( "createCoverFile", e,
            "creat file " + importCoverFile + " " + importOnceCoverFile,
            0, e );
    }
}

// create file
createCoverFile();

// before import importOnce
if( tmp1 !== 10 || foo1() !== 1 || tmp2 !== "a" || foo2() !== "abc" )
{
    throw buildException( null, null,
        "check var and func before import", "10 1 a abc",
        tmp1 + " " + foo1() + " " + tmp2 + " " + foo2() );
}

// import cover file
import( importCoverFile );
if( tmp1 !== 20 || foo1() !== 2 || tmp2 !== "a" || foo2() !== "abc" )
{
    throw buildException( null, null,
        "check var and func after import cover file", "20 2 a abc",
        tmp1 + " " + foo1() + " " + tmp2 + " " + foo2() );
}

// importOnce cover file
importOnce( importOnceCoverFile )
if( tmp1 !== 20 || foo1() !== 2 || tmp2 !== "b" || foo2() !== "def" )
{
    throw buildException( null, null,
        "check var and func after importOnce cover file", "20 2 b def",
        tmp1 + " " + foo1() + " " + tmp2 + " " + foo2() );
}

// remove file
removeFile( importCoverFile );
removeFile( importOnceCoverFile );