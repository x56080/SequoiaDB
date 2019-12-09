/*******************************************************************
*@Description : test nest import importOnce js file
*               seqDB-11904:使用import importOnce导入嵌套js文件
*@author      : Liang XueWang 
*******************************************************************/
// js file nest import, A import B
var nestImportFileA = WORKDIR + "/nestImportA_11904.js";
// js file nest import, B import A
var nestImportFileB = WORKDIR + "/nestImportB_11904.js";
// js file nest importOnce, A importOnce B
var nestImportOnceFileA = WORKDIR + "/nestImportOnceA_11904.js";
// js file nest importOnce, B importOnce A
var nestImportOnceFileB = WORKDIR + "/nestImportOnceB_11904.js";

function createNestImportFile ()
{
    try
    {
        var file = new File( nestImportFileA );
        file.write( "a++; import( \"" + nestImportFileB + "\" )" );
        file.close();
        file = new File( nestImportFileB );
        file.write( "a++; import( \"" + nestImportFileA + "\" )" );
        file.close();
    }
    catch( e )
    {
        throw buildException( "createNestImportFile", null, "create nest import file " +
            nestImportFileA + " " + nestImportFileB, 0, e );
    }

}

function createNestImportOnceFile ()
{
    try
    {
        var file = new File( nestImportOnceFileA );
        file.write( "b++; importOnce( \"" + nestImportOnceFileB + "\" )" );
        file.close();
        file = new File( nestImportOnceFileB );
        file.write( "b++; importOnce( \"" + nestImportOnceFileA + "\" )" );
        file.close();
    }
    catch( e )
    {
        throw buildException( "createNestImportOnceFile", null, "create nest importOnce file " +
            nestImportOnceFileA + " " + nestImportOnceFileB, 0, e );
    }
}

// create file
createNestImportFile();
createNestImportOnceFile();

// test nest import
var a = 0;
import( nestImportFileA );
if( a !== 2 )
{
    throw buildException( null, null,
        "test after import nest file: " + nestImportFileA + ", a value",
        2, a );
}

// test nest importOnce
var b = 0;
importOnce( nestImportOnceFileA );
if( b !== 2 )
{
    throw buildException( null, null,
        "test after importOnce nest file: " + nestImportOnceFileA + ", b value",
        2, b );
}

// remove file
removeFile( nestImportFileA );
removeFile( nestImportFileB );

removeFile( nestImportOnceFileA );
removeFile( nestImportOnceFileB );
