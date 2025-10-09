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
        rc = File.exist( fileName1 + '/error_time.csv' );
        assert.equal( rc, true );
        rc = File.exist( fileName1 + '/error_count.csv' );
        assert.equal( rc, true );

        log = diaglog.analyze().path( logPath1 + '.tar.gz' );
        fileName2 = log.run();
        rc = File.exist( fileName2 + '/error_time.csv' );
        assert.equal( rc, true );
        rc = File.exist( fileName2 + '/error_count.csv' );
        assert.equal( rc, true );

        // 要求结果一致
        rc = cmd.run( 'diff -q ' + fileName1 + '/error_time.csv ' + fileName2 + '/error_time.csv > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
        assert.equal( rc, '0' );
        rc = cmd.run( 'diff -q ' + fileName1 + '/error_count.csv ' + fileName2 + '/error_count.csv > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
        assert.equal( rc, '0' );

        log = diaglog.analyze().path( logPath2 );
        fileName1 = log.run();
        rc = File.exist( fileName1 + '/error_time.csv' );
        assert.equal( rc, true );
        rc = File.exist( fileName1 + '/error_count.csv' );
        assert.equal( rc, true );

        log = diaglog.analyze().path( logPath2 + '.zip' );
        fileName2 = log.run();
        rc = File.exist( fileName2 + '/error_time.csv' );
        assert.equal( rc, true );
        rc = File.exist( fileName2 + '/error_count.csv' );
        assert.equal( rc, true );

        // 要求结果一致
        rc = cmd.run( 'diff -q ' + fileName1 + '/error_time.csv ' + fileName2 + '/error_time.csv > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
        assert.equal( rc, '0' );
        rc = cmd.run( 'diff -q ' + fileName1 + '/error_count.csv ' + fileName2 + '/error_count.csv > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
        assert.equal( rc, '0' );
    } catch ( e ) {
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
        rc = File.exist( fileName + '/error_time.csv' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/error_count.csv' );
        assert.equal( rc, true );
    } catch ( e ) {
        throw e;
    }

    try {
        // 分析前面收集的日志
        diaglog.reset();
        log = diaglog.analyze().path( logPath + '.tar.gz' ).output( WORKDIR + '/diaglog_34329' );
        fileName = log.run();
        rc = File.exist( fileName + '/error_time.csv' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/error_count.csv' );
        assert.equal( rc, true );
    } catch ( e ) {
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

        var cmd = new Cmd();
        // 前面多次执行未指定输出目录，此时临时目录下应存在 10 个目录（上限）
        rc = cmd.run( 'ls -d /tmp/sequoiadb/analyze/diaglog_*.auto | wc -l' ).trimRight( '\n' );
        assert.equal( rc, '10' );
    } catch ( e ) {
        throw e;
    }
    diaglog.reset();
}

function test()
{
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