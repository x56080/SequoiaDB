/*******************************************************************
*@Description : test import importOnce no permission file
*               seqDB-11908:使用import importOnce导入无权限的文件
*@author      : Liang XueWang 
*******************************************************************/
// js file with no permission
var noPermFile  =  WORKDIR + "/noPerm_11908.js"

function createNoPermFile()
{
    try
    {
        var file = new File( noPermFile ) ;
        file.write( "var a = 1;" ) ;
        file.close() ;
        File.chmod( noPermFile, 0000 ) ;
    }
    catch( e )
    {
        throw buildException( "createNoPermFile", e,
              "create no perm file " + noPermFile, 0, e ) ;
    }    
}

function main()
{
	// check current user not root
	var user = currUser() ;
	if( user === "root" )  return ;

	// create no perm file
	createNoPermFile() ;

	// import importOnce no perm file
	try
	{
   	import( noPermFile ) ;
   	throw 0 ;
	}
	catch( e )
	{
   	if( e !== -3 )
    	{
      	throw buildException( null, null, "import no perm file", 13, e ) ;
    	}
	}

	try
	{
   	importOnce( noPermFile ) ;
    	throw 0 ;
	}
	catch( e )
	{
   	if( e !== -3 )
    	{
      	throw buildException( null, null, "importOnce no perm file", 13, e ) ;
    	}
	}

	// remove file
	removeFile( noPermFile ) ;
}

main() ;
