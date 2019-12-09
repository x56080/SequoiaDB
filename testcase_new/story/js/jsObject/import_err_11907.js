/*******************************************************************
*@Description : test import importOnce error
*               seqDB-11907:使用import importOnce导入不存在的文件
*               seqDB-11909:使用import importOnce导入非js文件
*@author      : Liang XueWang 
*******************************************************************/
// test import importOnce not exist file
var notExistFile = WORKDIR + "/notExist_11907.js";

try
{
    import( notExistFile );
    throw 0;
}
catch( e )
{
    if( e !== -4 )
    {
        throw buildException( null, null,
            "import not exist file " + notExistFile, -4, e );
    }
}

try
{
    importOnce( notExistFile );
    throw 0;
}
catch( e )
{
    if( e !== -4 )
    {
        throw buildException( null, null,
            "importOnce not exist file " + notExistFile, -4, e );
    }
}

// test import importOnce not js file( sdblobtool file )
var installPath = commGetInstallPath();
var sdblobtoolFile = installPath + "/bin/sdblobtool";

try
{
    importOnce( sdblobtoolFile );
    throw 0;
}
catch( e )
{
    if( e !== -152 )
    {
        throw buildException( null, null,
            "importOnce sdblobtool file " + sdblobtoolFile, -152, e );
    }
}

try
{
    import( sdblobtoolFile );
    throw 0;
}
catch( e )
{
    if( e !== -152 )
    {
        throw buildException( null, null,
            "import sdblobtool file " + sdblobtoolFile, -152, e );
    }
}
