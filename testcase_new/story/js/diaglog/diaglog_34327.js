/************************************
*@Description: seqDB-34327 search 功能基本参数测试
*@author:      jiangqiqian
*@createDate:  2025.09.15
**************************************/

main( test );

function testLastFile( diaglog )
{
    var log ;
    var fileName ;

    // 大于 0, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().lastFile( 1 ).keypattern( 'rc: ' ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().lastFile( 1 ).keypattern( 'rc: ' ).limit( 10 )");
        println('fileName: ' + fileName);
        throw e;
    }
    // 等于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().lastFile( 0 ).error( -79 ).limit( 1 );
    } );
    // 小于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().lastFile( -1 ).error( -79 ).limit( 1 );
    } );
    // 非数值, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().lastFile( '1' ).error( -79 ).limit( 1 );
    } );
    diaglog.reset();
}

function testLastest( diaglog )
{
    var log ;
    var fileName ;
    // 大于 0, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().lastest( 60 ).keypattern( 'rc: ' ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().lastest( 60 ).keypattern( 'rc: ' ).limit( 10 )");
        println('fileName: ' + fileName);
        throw e;
    }
    // 等于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().lastFile( 0 ).error( -79 ).limit( 1 );
    } );
    // 小于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().lastFile( -1 ).error( -79 ).limit( 1 );
    } );
    // 非数值, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().lastFile('1').error( -79 ).limit( 1 );
    } );
    diaglog.reset();
}

function testTimeBegin( diaglog )
{
    var log ;
    var fileName ;
    // 合法时间字符串, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().timeBegin( '2025-09-15T12:01:01.123456Z' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeBegin( '2025-09-15T12:01:01Z' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeBegin( '2025-09-15T12:01Z' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeBegin( '2025-09-15T12:01:01' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeBegin( '2025-09-15' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeBegin( '2025-09' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeBegin( '2025' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeBegin( '09/15/2025' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeBegin( 'Dec 15, 2025 12:01:01' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeBegin( 'Dec 15, 2025' ).keypattern( 'rc: ' ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().timeBegin(...)");
        println('fileName: ' + fileName);
        throw e;
    }
    // 非字符串, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().timeBegin( 1 ).error( -79 ).limit( 1 );
    } );
    // 非法时间字符串, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().timeBegin( '12345' ).error( -79 ).limit( 1 );
    } );
    // 非法时间字符串, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().timeBegin( 'abcde' ).error( -79 ).limit( 1 );
    } );
    diaglog.reset();
}

function testTimeEnd( diaglog )
{
    var log ;
    var fileName ;
    // 合法时间字符串, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().timeEnd( '9999-09-15T12:01:01.123456Z' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeEnd( '9999-09-15T12:01:01Z' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeEnd( '9999-09-15T12:01Z' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeEnd( '9999-09-15' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeEnd( '9999-09' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeEnd( '9999' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeEnd( '09/15/9999' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeEnd( 'Dec 15, 9999 12:01:01' ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().timeEnd( 'Dec 15, 9999' ).keypattern( 'rc: ' ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().timeEnd(...)");
        println('fileName: ' + fileName);
        throw e;
    }
    // 非字符串, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().timeEnd( 1 ).error( -79 ).limit( 1 );
    } );
    // 非法时间字符串, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().timeEnd( '12345' ).error( -79 ).limit( 1 );
    } );
    // 非法时间字符串, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().timeEnd( 'abcde' ).error( -79 ).limit( 1 );
    } );
    diaglog.reset();
}

function testError( diaglog )
{
    var log ;
    var fileName ;
    // 小于 0, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().error( -1 ).limit( 1 ).lastFile(1);
        fileName = log.run();
        log = diaglog.search().error( -16 ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );
        log = diaglog.search().error( -10000 ).limit( 1 ).lastFile(1);
        fileName = log.run();
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().error(...)");
        println('fileName: ' + fileName);
        throw e;
    }
    // 等于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( 0 ).limit( 1 );
    } );
    // 大于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( 1 ).limit( 1 );
    } );
    // 非数值, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( '-16' ).limit( 1 );
    } );
    diaglog.reset();
}

function testDiagLevel( diaglog )
{
    var log ;
    var fileName ;
    // 0-4, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().diaglevel( 0 ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().diaglevel( 1 ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().diaglevel( 2 ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().diaglevel( 3 ).keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        log = diaglog.search().diaglevel( 4 ).keypattern( 'rc: ' ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().diaglevel(...)");
        println('fileName: ' + fileName);
        throw e;
    }
    // 非 0-4, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().diaglevel(5).error( -79 ).limit( 1 );
    } );
    // 非数值, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().diaglevel('0').error( -79 ).limit( 1 );
    } );
    diaglog.reset();
}

function testKeypattern( diaglog )
{
    var log ;
    var fileName ;
    // 字符串, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().keypattern( 'Session' ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().keypattern( 'Session' ).limit( 10 )");
        println('fileName: ' + fileName);
        throw e;
    }
    // 非字符串, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().keypattern( -79 ).limit( 1 );
    } );
    diaglog.reset();
}

function testTid( diaglog )
{
    var log ;
    var fileName ;
    // 大于 0, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().tid( 12345 ).limit( 1 );
        fileName = log.run();
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.tid( 12345 ).limit( 1 )");
        println('fileName: ' + fileName);
        throw e;
    }
    // 等于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().tid( 0 ).limit( 1 );
    } );
    // 小于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().tid( -1 ).limit( 1 );
    } );
    // 非数值, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().tid( '12345' ).limit( 1 );
    } );
    diaglog.reset();
}

function testPid( diaglog )
{
    var log ;
    var fileName ;
    // 大于 0, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().pid( 12345 ).limit( 1 );
        fileName = log.run();
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().pid( 12345 ).limit( 1 )");
        println('fileName: ' + fileName);
        throw e;
    }
    // 等于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().pid( 0 ).limit( 1 );
    } );
    // 小于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().pid( -1 ).limit( 1 );
    } );
    // 非数值, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().pid( '12345' ).limit( 1 );
    } );
    diaglog.reset();
}

function testLimit( diaglog )
{
    var log ;
    var fileName ;
    // 不使用本函数，默认返回 100 条
    try {
        diaglog.reset();
        log = diaglog.search().keypattern( ' ' );
        fileName = log.run();
        var i = 0;
        while ( diaglog.next() ) {
            i++ ;
        }
        assert.equal(i, 100);
    } catch ( e ) {
        try {
            var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
            var cursor = db.exec( 'select diagpath from $SNAPSHOT_CONFIGS where role = ""' );
            while (cursor.next()) {
                var diagpath = cursor.current().toObj().diagpath;
                var cmd = new Cmd();
                cmd.run('cp -r ' + diagpath + ' /hdd/sequoiadb/');
            }
        } catch ( e ) {
            println("[ERROR] Failed to cp diaglog to /hdd/sequoiadb/");
        } finally {
            if (null != cursor) {
                cursor.close();
            }
            db.close();
        }

        println("[ERROR] Failed on diaglog.search().keypattern( ' ' ), test default limit(100)");
        println('fileName: ' + fileName);
        println('time: ' + Date());
        println('diaglog: ' + JSON.stringify(diaglog));
        throw e;
    }

    // 大于 0, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        var i = 0;
        while ( diaglog.next() ) {
            i++ ;
        }
        assert.equal(i, 1);
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().keypattern( 'a' ).limit( 1 )");
        println('fileName: ' + fileName);
        throw e;
    }
    // 等于 -1, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().lastFile( 1 ).keypattern( 'a' ).limit( -1 );
        fileName = log.run();
    } catch ( e ) {println("[ERROR] Failed on diaglog.search().lastFile( 1 ).keypattern( 'a' ).limit( -1 )");
        throw e;
    }
    // 等于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).limit( 0 );
    } );
    // 小于 -1, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).limit( -2 );
    } );
    // 非数值, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).limit( '1' );
    } );
    diaglog.reset();
}

function testOriginal( diaglog )
{
    var log ;
    var fileName ;
    // 结果为多行
    try {
        diaglog.reset();
        log = diaglog.search().keypattern( 'a' ).limit( 1 ).original();
        fileName = log.run();
        var array = diaglog.next().trimRight( '\n' ).split( 'Message:' );
        var result = array[0].split( '\n' );
        assert.equal( result.length, 6 );
        var message = array[1];
        assert.notEqual( message, '' );
        assert.notEqual( message, '\n' );

        log = diaglog.search().keypattern( 'a' ).limit( 10 ).original();
        fileName = log.run();
        testWithOriginal( diaglog );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().original(...), filename: " + fileName);
        println('fileName: ' + fileName);
        throw e;
    }
    // 结果为 1 行
    try {
        diaglog.reset();
        log = diaglog.search().keypattern( 'a' ).limit( 1 );
        fileName = log.run();
        var result = diaglog.next().trimRight( '\n' ).split( '\n' );
        assert.equal( result.length, 1 );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().original(...)");
        println('fileName: ' + fileName);
        throw e;
    }
}

function testAfter( diaglog )
{
    var log ;
    var fileName ;
    // 大于等于 0, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().keypattern( 'rc: ' ).after( 1 ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );

        log = diaglog.search().keypattern( 'a' ).after( 0 ).limit( 10 );
        fileName = log.run();
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().after(...)");
        println('fileName: ' + fileName);
        throw e;
    }
    // 小于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).after( -1 ).limit( 10 );
    } );
    // 非数值, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).after( '1' ).limit( 10 );
    } );
    diaglog.reset();
}

function testBefore( diaglog )
{
    var log ;
    var fileName ;
    // 大于等于 0, 预期成功
    try {
        diaglog.reset();
        log = diaglog.search().keypattern( 'rc: ' ).before( 1 ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );

        log = diaglog.search().keypattern( 'a' ).before( 0 ).limit( 10 );
        fileName = log.run();
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().before(...)");
        println('fileName: ' + fileName);
        throw e;
    }
    // 小于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).before( -1 ).limit( 10 );
    } );
    // 非数值, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).before( '1' ).limit( 10 );
    } );
    diaglog.reset();
}

function testNext ( diaglog )
{
    var log ;
    var fileName ;
    var result ;
    try {
        diaglog.reset();
        log = diaglog.search().keypattern( 'a' ).limit( 100 );
        fileName = log.run();
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().keypattern( 'a' ).limit( 100 )");
        throw e;
    }
    // 空, 预期成功
    try {
        result = diaglog.next() ;
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.next()");
        println('fileName: ' + fileName);
        throw e;
    }
    // 大于 1, 预期成功
    try {
        result = diaglog.next( 1 ) ;
        result = diaglog.next( 2 ) ;
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.next(...)");
        println('fileName: ' + fileName);
        throw e;
    }

    // 等于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        result = diaglog.next( 0 ) ;
    } );
    // 小于 0, 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        result = diaglog.next( -1 ) ;
    } );
    // 非数值 , 预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        result = diaglog.next( '1' ) ;
    } );
    diaglog.reset();
}

function testMultiple ( diaglog )
{
    var log ;
    var fileName ;
    // 起始时间和结束时间不冲突，预期成功
    try {
        diaglog.reset();
        log = diaglog.search().timeBegin( '2025-09-15T12:01:01.123456Z' ).timeEnd( '9999-09-15T12:01:01.123456Z' ).keypattern( 'rc: ' ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().timeBegin( '2025-09-15T12:01:01.123456Z' ).timeEnd( '9999-09-15T12:01:01.123456Z' ).keypattern( 'rc: ' ).limit( 10 )");
        println('fileName: ' + fileName);
        throw e;
    }

    // 同时指定错误和关键字搜索，预期成功
    try {
        diaglog.reset();
        log = diaglog.search().error( -16 ).keypattern( 'rc: ' ).limit( 10 );
        fileName = log.run();
        testWithoutOriginal( diaglog );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().error( -16 ).keypattern( 'rc: ' ).limit( 10 )");
        println('fileName: ' + fileName);
        throw e;
    }

    // 起始时间和结束时间冲突，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().timeEnd( '2025-09-15T12:01:01.123456Z' ).timeBegin( '9999-09-15T12:01:01.123456Z' ).error( -79 ).limit( 1 );
    } );
    diaglog.reset();
}

function testOutput( diaglog )
{
    var log ;
    var fileName ;
    var rc;
    try {
        var cmd = new Cmd();
        // 前面多次执行未指定输出目录，此时临时目录下应存在 10 个目录（上限）
        rc = cmd.run( 'ls -d /tmp/sequoiadb/search/cluster*.auto | wc -l' ).trimRight( '\n' );
        assert.equal( rc, '10' );
    } catch ( e ) {
        println("[ERROR] Failed on check /tmp/sequoiadb/search/cluster*.auto");
        throw e;
    }

    // 指定 output 目录执行，生成的目录数不受限制，可超过 10 个目录上限
    var fileName;
    try {
        diaglog.reset();
        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_1' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_2' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );
    
        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_3' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_4' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_5' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_6' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_7' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_8' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_9' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_10' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_11' );
        fileName = log.run();
        rc = File.exist( fileName );
        assert.equal( rc, true );

        rc = cmd.run( 'ls -d ' + WORKDIR + '/diaglog_34327/* | wc -l' ).trimRight( '\n' );
        assert.equal( rc, '11' );
    } catch ( e ) {
        println("[ERROR] Failed on check " + WORKDIR + "/diaglog_34327/*");
        throw e;
    }

    // 搜索读取后再次搜索，结果写入同一文件
    try {
        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_12' );
        fileName = log.run();
        diaglog.next();
        log = diaglog.search().keypattern( 'a' ).limit( 1 ).output(  WORKDIR + '/diaglog_34327/result_12' );
        fileName = log.run();
        diaglog.next();
    } catch ( e ) {
        println("[ERROR] Failed on check " + WORKDIR + "/diaglog_34327/*");
        throw e;
    }

    // 指定非字符串，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).limit( 1 ).output( 123 );
    } );

    // 指定非绝对路径，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).limit( 1 ).output( './diaglog_34327/result_12' );
    } );

    // 指定空字符串，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).limit( 1 ).output( '' );
    } );
    diaglog.reset();
}

function testPath( diaglog )
{
    var log;
    var fileName;

    // 指定当前节点的 diaglog 目录，预期成功
    try {
        var oma = new Oma( COORDHOSTNAME, CMSVCNAME );
        var dataPath = JSON.parse( oma.listNodes()[0] ).dbpath;
        diaglog.reset();
        log = diaglog.search().keypattern( 'a' ).limit( 1 ).path( dataPath + '/diaglog' );
        fileName = log.run();
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().keypattern( 'a' ).limit( 1 ).path( '" + dataPath + "/diaglog' )");
        println('fileName: ' + fileName);
        throw e;
    } finally {
        if ( null != oma ){
            oma.close();
        }
    }

    // 指定存在的非 diaglog 目录，预期成功（搜索无结果，但不算失败）
    try {
        log = diaglog.search().error( -79 ).limit( 1 ).path( '/tmp' );
        fileName = log.run();
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.search().error( -79 ).limit( 1 ).path( '/tmp' )");
        throw e;
    }    

    // 指定不存在的目录，预期失败
    assert.tryThrow( SDB_FNE, function()
    {
        log = diaglog.search().error( -79 ).limit( 1 ).path( WORKDIR + '/diaglog_34327/abcde' );
        fileName = log.run();
    } );

    // 指定非字符串，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).limit( 1 ).path( 123 );
    } );

    // 指定非绝对路径，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).limit( 1 ).path( './diaglog_34327/result_12' );
    } );

    // 指定空字符串，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.search().error( -79 ).limit( 1 ).path( '' );
    } );
    diaglog.reset();
}

function testWithoutOriginal( diaglog )
{
    // 检查简要模式下查询的结果是否按时间降序，以及各字段内容是否正常有值
    try {
        var line = "";
        var preTime = "999999999999999999999999999";
        var curTime = "";
        var array = [];
        var rc;
        while ( line = diaglog.next() )
        {
            array = line.split(',');
            rc = array.length >= 5;
            assert.equal( rc, true );
            for ( i in array ){
                assert.notEqual( array[i], "");
            }
            curTime = array[3];
            rc = curTime <= preTime;
            assert.equal( rc, true );
            preTime = curTime;
        }
    } catch ( e ) {
        println("[ERROR] Failed on testWithoutOriginal()");
        println('array: ' + JSON.stringify(array));
        println('preTime: ' + preTime);
        throw e ;
    }
}

function testWithOriginal( diaglog )
{
    // 检查原始模式下查询的结果是否按时间降序，以及各行内容是否正常有值
    try {
        var line = "";
        var preTime = "999999999999999999999999999";
        var curTime = "";
        var array = [];
        var rc;
        while ( line = diaglog.next() )
        {
            array = line.split('\n');
            rc = array.length >= 7;
            assert.equal( rc, true );
            for (let i = 0; i < 7; i++){
                assert.notEqual( array[i], "" );
            }

            curTime = array[1].split(' ')[0];
            rc = curTime <= preTime;
            assert.equal( rc, true );
            preTime = curTime;
        }
    } catch ( e ) {
        println("[ERROR] Failed on testWithOriginal()");
        println('array: ' + JSON.stringify(array));
        println('preTime: ' + preTime);
        throw e ;
    }
}

function test()
{
    try {
        // 删除可能残留用例目录（上次测试用例运行失败）
        File.remove( WORKDIR + '/diaglog_34327' );
    } catch (e) {}

    try {
        var diaglog = new DiagLog( COORDHOSTNAME, COORDSVCNAME );

        // lastFile
        testLastFile( diaglog );

        // lastest
        testLastest( diaglog );

        // timeBegin
        testTimeBegin( diaglog );

        // timeEnd
        testTimeEnd( diaglog );

        // error
        testError( diaglog );

        // diaglevel
        testDiagLevel( diaglog );

        // keypattern
        testKeypattern( diaglog );

        // tid
        testTid( diaglog );

        // pid
        testPid( diaglog );

        // limit
        testLimit( diaglog );

        // original
        testOriginal( diaglog );

        // after
        testAfter( diaglog );

        // before
        testBefore( diaglog );

        // next
        testNext( diaglog );

        // output
        testOutput( diaglog );

        // path
        testPath( diaglog );

        // 多个参数复合测试
        testMultiple( diaglog );

        try {
            File.remove( WORKDIR + '/diaglog_34327' );
        } catch (e) {
            println("[ERROR] Failed on remove '" + WORKDIR + "/diaglog_34327'");
            throw e;
        }
    } catch (e) {
        throw e;
    } finally {
        if ( null != diaglog ) {
            diaglog.close();
        }
    }
}