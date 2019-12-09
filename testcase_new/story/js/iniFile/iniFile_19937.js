/************************************
*@Description: seqDB-19937 IniFile类SDB_INIFILE_FLAGS_MYSQL测试 
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
    var filePath = WORKDIR + "/ini19937/";
    var fileName = "file19937";
    var fileFullPath = filePath + fileName;
    makeIniFile( filePath, fileName );

    var section1 = "section1";
    var key1 = "key1";
    var value1 = "value1";
    var comment1 = "comment1";

    // 校验默认配置为: SDB_INIFILE_HASHMARK SDB_INIFILE_EQUALSIGN SDB_INIFILE_STRICTMODE 
    // 检查 SDB_INIFILE_HASHMARK SDB_INIFILE_EQUALSIGN
    var iniFile = new IniFile( fileFullPath, SDB_INIFILE_FLAGS_MYSQL );
    iniFile.setValue( section1, key1, value1 );
    iniFile.setComment( section1, key1, comment1 );
    iniFile.save();

    var checkFile = new IniFile( fileFullPath, SDB_INIFILE_FLAGS_MYSQL );
    var fileContent = "[" + section1 + "]\n" +
        "# " + comment1 + "\n" +
        key1 + "=" + value1;
    var checkValue1 = checkFile.toString();
    compareValue( fileContent, checkValue1 );
    deleteIniFile( fileFullPath );

    // 检查 SDB_INIFILE_STRICTMODE
    makeIniFile( filePath, fileName );
    var fileContent = "[" + section1 + "]\n" +
        key1 + "=" + value1 + "\n" +
        key1 + "=" + value1;
    initFile( fileFullPath, fileContent );
    try
    {
        var iniFile = new IniFile( fileFullPath, SDB_INIFILE_FLAGS_MYSQL );
        throw new Error( "check SDB_INIFILE_STRICTMODE need fail" );
    } catch( e )
    {
        if( e !== -6 )
        {
            throw new Error( "expect -6 but throw " + e );
        }
    }

    deleteIniFile( filePath );
}
