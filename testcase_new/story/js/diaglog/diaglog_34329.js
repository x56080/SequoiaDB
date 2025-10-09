/************************************
*@Description: seqDB-34329 analyze 功能基本参数测试
*@author:      jiangqiqian
*@createDate:  2025.09.15
**************************************/

main( test );

function collect( diaglog, compress )
{
    var log;
    var fileName;
    // 执行 collect 用于后续的 analyze
    try {
        diaglog.reset();
        log = diaglog.collect().keypattern( 'rc: ' ).lastFile( 1 ).compress( compress );
        fileName = log.run();
    } catch (error) {
        println("[ERROR] Failed on diaglog.collect().keypattern( 'rc: ' ).lastFile( 1 ).compress( compress )");
        throw e;
    }
    diaglog.reset();
    return fileName ;
}

function testPath( diaglog, logPath1, logPath2 )
{
    var log;
    var fileName1;
    var fileName2;
    var rc;
    try {
        var cmd = new Cmd() ;
        // 分析前面收集的日志
        diaglog.reset();
        log = diaglog.analyze().path( logPath1 );
        fileName1 = log.run();
        testCsv(fileName1);

        log = diaglog.analyze().path( logPath1 + '.tar.gz' );
        fileName2 = log.run();
        testCsv(fileName2);

        // 要求结果一致
        rc = cmd.run( 'diff -q ' + fileName1 + '/error_time.csv ' + fileName2 + '/error_time.csv > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
        assert.equal( rc, '0' );
        rc = cmd.run( 'diff -q ' + fileName1 + '/error_count.csv ' + fileName2 + '/error_count.csv > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
        assert.equal( rc, '0' );

        log = diaglog.analyze().path( logPath2 + '/' );
        fileName1 = log.run();
        testCsv(fileName1);

        log = diaglog.analyze().path( logPath2 + '.zip' );
        fileName2 = log.run();
        testCsv(fileName2);

        // 要求结果一致
        rc = cmd.run( 'diff -q ' + fileName1 + '/error_time.csv ' + fileName2 + '/error_time.csv > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
        assert.equal( rc, '0' );
        rc = cmd.run( 'diff -q ' + fileName1 + '/error_count.csv ' + fileName2 + '/error_count.csv > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
        assert.equal( rc, '0' );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.analyze().path()");
        throw e;
    }
    diaglog.reset();
}

function testOutput( diaglog, logPath )
{
    var log;
    var fileName;
    var rc;
    try {
        // 分析前面收集的日志
        diaglog.reset();
        log = diaglog.analyze().path( logPath ).output( WORKDIR + '/diaglog_34329' );
        fileName = log.run();
        testCsv(fileName);
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.analyze().path( " + logPath + " ).output( '" + WORKDIR + "/diaglog_34329' )");
        throw e;
    }

    try {
        // 分析前面收集的日志
        diaglog.reset();
        log = diaglog.analyze().path( logPath + '.tar.gz' ).output( WORKDIR + '/diaglog_34329' );
        fileName = log.run();
        testCsv(fileName);
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.analyze().path( " + logPath + ".tar.gz ).output( '" + WORKDIR + "/diaglog_34329' )");
        throw e;
    }

    try {
        // 分析前面收集的日志
        diaglog.reset();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        diaglog.analyze().path( logPath ).run();
        testCsv(fileName);

        var cmd = new Cmd();
        // 前面多次执行未指定输出目录，此时临时目录下应存在 10 个目录（上限）
        rc = cmd.run( 'ls -d /tmp/sequoiadb/analyze/diaglog_*.auto | wc -l' ).trimRight( '\n' );
        assert.equal( rc, '10' );
    } catch ( e ) {
        println("[ERROR] Failed on diaglog.analyze().path( " + logPath + " ).run()");
        throw e;
    }
    diaglog.reset();
}

function testCsv( path )
{
    // 检查 analyze 产生的 csv 格式是否正常
    try {
        // 检查文件是否存在
        rc = File.exist( path + '/error_time.csv' );
        assert.equal( rc, true );
        rc = File.exist( path + '/error_count.csv' );
        assert.equal( rc, true );

        // 校验格式
        var file = new File( path + '/error_time.csv', 0644, SDB_FILE_READONLY );
        var isFirstLine = true;
        var preTime = "9999-12-31-01.01.01.000000";
        var curTime = "";
        var count = 0;
        try {
            while ( line = file.readLine().trimRight( '\n' ) ) {
                if ( isFirstLine ) {
                    assert.equal( line, 'HostName,ServiceName,GroupName,Error,Time' );
                    isFirstLine = false;
                } else {
                    count++;
                    assert.equal( line.split(',').length, 5 );
                    // 按时间降序
                    curTime = line.split(',')[4];
                    rc = curTime <= preTime;
                    assert.equal( rc, true );
                    preTime = curTime;
                }
            }
        } catch ( e  ) {
            if ( -9 != e ) {
                println("[ERROR] Failed on testCsv()");
                println('curTime:' + curTime);
                println('preTime:' + preTime);
                println('fileName: ' + path + '/error_time.csv');
                throw e ;
            }
        } finally {
            file.close() ;
        }
        // 必须有结果
        assert.notEqual( count, 0 );

        var file = new File( path + '/error_count.csv', 0644, SDB_FILE_READONLY );
        var isFirstLine = true;
        var count = 0;
        var errorObj = {};
        try {
            while ( line = file.readLine().trimRight( '\n' ) ) {
                if ( isFirstLine ) {
                    assert.equal( line, 'HostName,ServiceName,GroupName,Error,Count' );
                    isFirstLine = false;
                } else {
                    count++;
                    var array = line.split(',');
                    assert.equal( array.length, 5 );
                    // 不会出现重复的 ERROR
                    var key = array[0] + '_' + array[1] + '_' + array[2] + '_' + array[3];
                    assert.notEqual( errorObj[key], 1 );
                    errorObj[key] = 1;
                }
            }
        } catch ( e  ) {
            if ( -9 != e ) {
                println("[ERROR] Failed on testCsv()");
                println("array: " + JSON.stringify(array));
                println("errorObj: " + JSON.stringify(errorObj));
                throw e ;
            }
        } finally {
            file.close() ;
        }
        // 必须有结果
        assert.notEqual( count, 0 );
    } catch (e) {
        throw e;
    }
}

function test()
{
    try {
        // 删除可能残留用例目录（上次测试用例运行失败）
        File.remove( WORKDIR + '/diaglog_34329' );
    } catch (e) {}

    try {
        var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
        var diaglog = new DiagLog( COORDHOSTNAME, COORDSVCNAME );
        var logPath1 = collect( diaglog, 'tar.gz' );
        var logPath2 = collect( diaglog, 'zip' );

        // path
        testPath( diaglog, logPath1, logPath2 );

        // output
        testOutput( diaglog, logPath1 );

        File.remove( WORKDIR + '/diaglog_34329' );
    } catch (e) {
        throw e;
    } finally {
        if ( null != db ) {
            db.close();
        }
        if ( null != diaglog ) {
            diaglog.close();
        }
    }
}