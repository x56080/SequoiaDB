/*******************************************************************
*@Description : test import importOnce empty js file
*               seqDB-12815:使用import importOnce导入空文件
*@author      : Liang XueWang 
*******************************************************************/
// empty file to import
var emptyImportFile     = WORKDIR + "/emptyImport_12815.js" ; 
// empty file to importOnce    
var emptyImportOnceFile = WORKDIR + "/emptyImportOnce_12815.js" ; 

function createEmptyImportFile()
{
    try
    {
        var file = new File( emptyImportFile ) ;
        file.close() ;
    }
    catch( e )
    {
        throw buildException( "createEmptyImportFile", null, 
              "create empty import file " + emptyImportFile, 0, e ) ;
    }
}

function createEmptyImportOnceFile()
{
    try
    {
        var file = new File( emptyImportOnceFile ) ;
        file.close() ;
    }
    catch( e )
    {
        throw buildException( "createEmptyImportOnceFile", null, 
              "create empty importOnce file " + emptyImportOnceFile, 0, e ) ;
    }  
}

// creat empty import importOnce file
createEmptyImportFile() ;
createEmptyImportOnceFile() ;

// test import empty file
try
{
    import( emptyImportFile ) ;
}
catch( e )
{
    throw buildException( null, null, "test import empty file", 0, e ) ;
}

// test importOnce empty file
try
{
    importOnce( emptyImportOnceFile ) ;
}
catch( e )
{
    throw buildException( null, null, "test importOnce empty file", 0, e ) ;
}     

// remove file   
removeFile( emptyImportFile ) ;
removeFile( emptyImportOnceFile ) ;    