/************************************
*@Description: seqDB-34330 测试 search、collect、analyze 功能相互覆盖使用
*@author:      jiangqiqian
*@createDate:  2025.09.29
**************************************/

main( test );

function test()
{
    try {
        var diaglog = new DiagLog( COORDHOSTNAME, COORDSVCNAME );
        var log;
        var fileName;
        var fileName1;
        var fileName2;
        var rc;
        try {
            // 搜索日志
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 );
            fileName = log.run();
            rc = File.exist( fileName );
            assert.equal( rc, true );

            // 收集日志
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).collect();
            fileName = log.run();
            rc = File.exist( fileName );
            assert.equal( rc, true );
            rc = File.exist( fileName + '.tar.gz' );
            assert.equal( rc, true );

            // 分析前面收集的日志
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).collect().analyze().path( fileName );
            fileName = log.run();
            rc = File.exist( fileName + '/error_time.csv' );
            assert.equal( rc, true );
            rc = File.exist( fileName + '/error_count.csv' );
            assert.equal( rc, true );

            // 测试不 reset 是否能沿用续用变量
            // 限制时间为当前时间之前，保证搜索的日志结果相同
            var date = new Date();
            date.setHours(date.getHours() - 2);
            var timeStr = date.toString();

            diaglog.reset();
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).timeEnd( timeStr );
            fileName1 = log.run();
            rc = File.exist( fileName1 );
            assert.equal( rc, true );

            log = diaglog.collect();
            fileName = log.run();
            rc = File.exist( fileName );
            assert.equal( rc, true );
            rc = File.exist( fileName + '.tar.gz' );
            assert.equal( rc, true );

            log = diaglog.search().path( fileName );
            fileName2 = log.run();
            rc = File.exist( fileName2 );
            assert.equal( rc, true );

            // 排除文件名影响
            cmd.run( 'sed -i "s#,/[^,]*,#,#g" ' + fileName1 );
            cmd.run( 'sed -i "s#,/[^,]*,#,#g" ' + fileName2 );
            rc = cmd.run( 'diff -q ' + fileName1 + ' ' + fileName2 + ' > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
            assert.equal( rc, '0' );

            log = diaglog.analyze();
            fileName = log.run();
            rc = File.exist( fileName + '/error_time.csv' );
            assert.equal( rc, true );
            rc = File.exist( fileName + '/error_count.csv' );
            assert.equal( rc, true );
        } catch ( e ) {
            println('filename: ' + fileName);
            println('filename1: ' + fileName1);
            println('filename2: ' + fileName2);
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