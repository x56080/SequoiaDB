/************************************
*@Description: seqDB-19545 指定不存在的item设置注释
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
    var filePath = WORKDIR + "/ini19545/";
    var fileName = "file19545";
    var fileFullPath = filePath + fileName;
    makeIniFile( filePath, fileName );

    var section1 = "section1";
    var key1 = "key1";
    var value1 = "value1";
    var comment1 = "comment1";
    initFile( fileFullPath, "[" + section1 + "]" );

    // 指定不存在的item设置注释：指定section进行设置 不指定section进行设置
    var iniFile = new IniFile( fileFullPath );
    try
    {
        iniFile.setComment( section1, key1, comment1 );
        throw new Error( "set item comment need fail" );
    } catch( e )
    {
        if( e !== -211 )
        {
            throw new Error( "expect throw -211 but throw " + e );
        }
    }

    try
    {
        iniFile.setComment( key1, comment1 );
        throw new Error( "set item comment need fail" );
    } catch( e )
    {
        if( e !== -211 )
        {
            throw new Error( "expect throw -211 but throw " + e );
        }
    }

    // 存在该item已注释：指定section进行设置 不指定section进行设置
    deleteIniFile( fileFullPath );
    makeIniFile( filePath, fileName );
    var fileContent = "; " + key1 + "=" + value1 + "\n" +
        "[" + section1 + "]\n" +
        "; " + key1 + "=" + value1;
    initFile( fileFullPath, fileContent );

    var iniFile = new IniFile( fileFullPath );
    try
    {
        iniFile.setComment( section1, key1, comment1 );
        throw new Error( "set item comment need fail" );
    } catch( e )
    {
        if( e !== -211 )
        {
            throw new Error( "expect throw -211 but throw " + e );
        }
    }

    try
    {
        iniFile.setComment( key1, comment1 );
        throw new Error( "set item comment need fail" );
    } catch( e )
    {
        if( e !== -211 )
        {
            throw new Error( "expect throw -211 but throw " + e );
        }
    }

    deleteIniFile( filePath );
}
