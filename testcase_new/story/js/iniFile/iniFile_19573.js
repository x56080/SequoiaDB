/************************************
*@Description: seqDB-19573 注释不存在的item
*@author:      yinzhen
*@createDate:  2019.10.12
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
    var filePath = WORKDIR + "/ini19573/";
    var fileName = "file19573";
    var fileFullPath = filePath + fileName;
    makeIniFile( filePath, fileName );

    var section1 = "section1";
    var key1 = "key1";
    var value1 = "value1";
    var fileContent = "[" + section1 + "]\n" +
        key1 + "=" + value1;
    initFile( fileFullPath, fileContent );

    // 注释不存在的item 指定section进行设置 
    var iniFile = new IniFile( fileFullPath );
    try
    {
        iniFile.disableItem( section1, "key2" );
        throw new Error( "disable item need throw error" );
    } catch( e )
    {
        if( e !== -211 )
        {
            throw new Error( "except throw -211 but throw " + e );
        }
    }

    // 不指定section进行设置
    try
    {
        iniFile.disableItem( "key2" );
        throw new Error( "disable item need throw error" );
    } catch( e )
    {
        if( e !== -211 )
        {
            throw new Error( "except throw -211 but throw " + e );
        }
    }

    // 存在该item已注释 
    var fileContent = "; key2=value2\n" +
        "[" + section1 + "]\n" +
        "; key2=value2\n" +
        key1 + "=" + value1;
    deleteIniFile( fileFullPath );
    makeIniFile( filePath, fileName );
    initFile( fileFullPath, fileContent );

    // 注释不存在的item 指定section进行设置 
    var iniFile = new IniFile( fileFullPath );
    try
    {
        iniFile.disableItem( section1, "key2" );
        throw new Error( "disable item need throw error" );
    } catch( e )
    {
        if( e !== -211 )
        {
            throw new Error( "except throw -211 but throw " + e );
        }
    }

    // 不指定section进行设置
    try
    {
        iniFile.disableItem( "key2" );
        throw new Error( "disable item need throw error" );
    } catch( e )
    {
        if( e !== -211 )
        {
            throw new Error( "except throw -211 but throw " + e );
        }
    }

    deleteIniFile( filePath );
}
