/************************************
*@Description: seqDB-19943 打开不存在的ini文件
*@author:      yinzhen
*@createDate:  2019.10.11
**************************************/
try
{
    main();
}
catch( e )
{
    if( e.constructor === Error )
    {
        println( e.stack );
    }
    throw e;
}

function main ()
{
    var filePath = WORKDIR + "/ini19943/";
    var fileName = "file19943";
    var fileFullPath = filePath + fileName;

    try
    {
        var iniFile = new IniFile( fileFullPath );
        throw new Error( "open not exist file need throw error" );
    } catch( e )
    {
        if( e !== -4 )
        {
            throw new Error( "expect throw -4 but throw " + e );
        }
    }
}
