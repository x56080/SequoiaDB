/*******************************************************************
*@Description : test repeat import importOnce js file
*               seqDB-11905:使用import重复导入文件
*               seqDB-11906:使用importOnce重复导入文件
*@author      : Liang XueWang 
*******************************************************************/
// js file to repeat import
var repeatImportFile     = WORKDIR + "/repeatImport_11905.js" ; 
// js file to repeat importOnce    
var repeatImportOnceFile = WORKDIR + "/repearImportOnce_11906.js" ; 

function createRepeatImportFile()
{
    try
    {
        var file = new File( repeatImportFile ) ;
        file.write( "a++; function sub( x, y) { return x - y ; }" ) ;
        file.close() ;
    }
    catch( e )
    {
        throw buildException( "createRepeatImportFile", null, 
              "create repeat import file " + repeatImportFile, 0, e ) ;
    } 
}

function createRepeatImportOnceFile()
{
    try
    {
        var file = new File( repeatImportOnceFile ) ;
        file.write( "b++; function divide( x, y ) { return x / y ; }" ) ;
        file.close() ;
    }
    catch( e )
    {
        throw buildException( "createRepeatImportOnceFile", null, 
              "create repeat importOnce file " + repeatImportOnceFile, 0, e ) ;
    }  
}

// creat repeat import importOnce file
createRepeatImportFile() ;
createRepeatImportOnceFile() ;

// test repeat import
var a = 0 ;
import( repeatImportFile ) ;
import( repeatImportFile ) ;
if( a !== 2 )
{
    throw buildException( null, null, 
          "test after repeat import, a value", 2, a ) ;
}    

// test repeat importOnce
var b = 0 ;
importOnce( repeatImportOnceFile ) ;
importOnce( repeatImportOnceFile ) ;
if( b !== 1 )
{
    throw buildException( null, null,
          "test after repeat importOnce, b value", 1, b ) ;
}        

// remove repeat repeatOnce file   
removeFile( repeatImportFile ) ;
removeFile( repeatImportOnceFile ) ;    