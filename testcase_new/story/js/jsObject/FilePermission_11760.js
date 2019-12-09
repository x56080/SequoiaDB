/******************************************************************************
*@Description : test js object File function: open file with permission mode
*               TestLink : 11760 文件参数 permission mode校验
*@auhor       : Liang XueWang
******************************************************************************/
// get remote host and connect
try
{
    var remotehost = toolGetRemotehost();
    remotehost = remotehost["hostname"];
    var remote = new Remote( remotehost, CMSVCNAME );
    println( "init remote with remote host: " + remotehost );
}
catch( e )
{
    throw buildException( null, null, e,
        "connect remote host " + remotehost + " in the begining",
        0, e );
}

function testInitNotExistFile ( local )
{
    var filename = "/tmp/initNotExist_11760.js";
    var cmd;
    var command = "rm -rf " + filename;
    var user;

    if( local )
    {
        cmd = new Cmd();
        user = cmd.run( "whoami" ).split( "\n" )[0];
    }
    else
    {
        cmd = remote.getCmd();
        user = toolGetSdbcmUser( remotehost, CMSVCNAME );
    }
    cmd.run( command );

    // test init not exist file with permission and mode
    var permissions = [0, 0644, 0755, 0755, 0755, 0755, 0755, 0755, 0444];
    var modes = [0,
        0,
        SDB_FILE_READWRITE | SDB_FILE_CREATEONLY,
        SDB_FILE_READONLY | SDB_FILE_CREATEONLY,
        SDB_FILE_WRITEONLY | SDB_FILE_CREATEONLY,
        SDB_FILE_READWRITE,
        SDB_FILE_READWRITE | SDB_FILE_REPLACE,
        SDB_FILE_READWRITE | SDB_FILE_CREATE,
        SDB_FILE_READWRITE | SDB_FILE_CREATE];
    // errno of open file, read file, write file
    var errnos = [[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, -9, -3], [0, -3, 0],
    [-4, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]];

    for( var i = 0; i < permissions.length; i++ )
    {
        var per = permissions[i];
        var mode = modes[i];
        var errno = errnos[i];
        toolInitFile( local, filename, per, mode, errno );
        cmd.run( command );
    }
}

function testInitExistedFile ( local )
{
    var filename = "/tmp/initExist_11760.js";
    var file, cmd;
    var command = "rm -rf " + filename;
    if( local )  
    {
        cmd = new Cmd();
        cmd.run( command );
        file = new File( filename, 0644 );
    }
    else
    {
        cmd = remote.getCmd();
        cmd.run( command );
        file = remote.getFile( filename, 0644 );
    }
    file.close();

    var permissions = [0, 0755, 0755];
    var modes = [0, 0, SDB_FILE_READONLY];
    var errnos = [[0, 0, 0], [0, 0, 0], [0, 0, -3]];

    for( var i = 0; i < permissions.length; i++ )
    {
        var per = permissions[i];
        var mode = modes[i];
        var errno = errnos[i];
        toolInitFile( local, filename, per, mode, errno );
    }

    cmd.run( command );
}

function toolInitFile ( local, filename, per, mode, errno )
{
    var file;
    var host = local ? "localhost" : remotehost;
    var openErr = errno[0];
    var readErr = errno[1];
    var writeErr = errno[2];

    println( "test file " + filename + " with per: " + per +
        " mode: " + mode + " in host: " + host );

    // test init
    try
    {
        if( local )  
        {
            if( per == 0 && mode == 0 )
                file = new File( filename );
            else if( mode == 0 )
                file = new File( filename, per );
            else
                file = new File( filename, per, mode );
        }
        else
        {
            if( per == 0 && mode == 0 )
                file = remote.getFile( filename );
            else if( mode == 0 )
                file = remote.getFile( filename, per );
            else
                file = remote.getFile( filename, per, mode );
        }
        if( openErr !== 0 )
        {
            throw openErr;
        }
    }
    catch( e )
    {
        if( e === openErr ) return;
        else
        {
            throw buildException( "toolInitFile", e, "init file", openErr, e );
        }
    }

    // test write
    try
    {
        file.write( "abc" );
        if( writeErr !== 0 )
        {
            throw writeErr;
        }
    }
    catch( e )
    {
        if( e === writeErr );
        else
        {
            throw buildException( "toolInitFile", e, "write file", writeErr, e );
        }
    }

    // test read
    try
    {
        file.seek( 0, "b" );
        file.read();
        if( readErr !== 0 )
        {
            throw readErr;
        }
    }
    catch( e )
    {
        if( e === readErr );
        else
        {
            throw buildException( "toolInitFile", e, "read file", readErr, e );
        }
    }

    file.close();
}

function main ()
{
    testInitNotExistFile( true );
    testInitNotExistFile( false );
    testInitExistedFile( true );
    testInitExistedFile( false );
}

main();

remote.close();