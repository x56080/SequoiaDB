/************************************
*@Description: seqDB-34330 测试 search、collect、analyze 功能相互覆盖使用
*@author:      jiangqiqian
*@createDate:  2025.09.29
**************************************/

main( test );

function test()
{
    try {
        // 删除可能残留用例目录（上次测试用例运行失败）
        File.remove( WORKDIR + '/diaglog_34330' );
    } catch (e) {}

    try {
        var diaglog = new DiagLog( COORDHOSTNAME, COORDSVCNAME );
        var log;
        var fileName;
        var fileName1;
        var fileName2;
        var packageName;
        var rc;
        try {
            // 搜索日志
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 );
            fileName = log.run();
            rc = File.exist( fileName );
            assert.equal( rc, true );
            testWithoutOriginal(diaglog);

            // 收集日志
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).collect();
            fileName = log.run();
            rc = File.exist( fileName );
            assert.equal( rc, true );
            rc = File.exist( fileName + '.tar.gz' );
            assert.equal( rc, true );
            packageName = fileName + '.tar.gz';

            // 分析前面收集的日志
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).collect().analyze().path( fileName );
            fileName = log.run();
            testCsv(fileName);

            // 分析 tar.gz 压缩包
            diaglog.reset();
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).collect().analyze().path( packageName ).output( WORKDIR + '/diaglog_34330' );
            testCsv(fileName);

            // 搜索 tar.gz 压缩包
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).path( packageName ).output( WORKDIR + '/diaglog_34330/' );
            fileName = log.run();
            rc = File.exist( fileName );
            assert.equal( rc, true );
            testWithoutOriginal(diaglog);

            // 收集 zip 压缩包
            diaglog.reset();
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).collect().compress( 'zip' );
            fileName = log.run();
            rc = File.exist( fileName );
            assert.equal( rc, true );
            rc = File.exist( fileName + '.zip' );
            assert.equal( rc, true );
            packageName = fileName + '.zip';

            // 分析 zip 压缩包
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).collect().analyze().path( packageName ).output( WORKDIR + '/diaglog_34330/' );
            fileName = log.run();
            testCsv(fileName);

            // 搜索 zip 压缩包
            diaglog.reset();
            log = diaglog.search().keypattern( 'rc: ' ).lastFile( 1 ).path( packageName ).output( WORKDIR + '/diaglog_34330' );
            fileName = log.run();
            rc = File.exist( fileName );
            assert.equal( rc, true );
            testWithoutOriginal(diaglog);

            // 测试不 reset 是否能沿用续用变量
            // 限制时间为当前时间之前 5 分钟，以保证搜索的日志结果相同
            var date = new Date();
            date.setMinutes(date.getMinutes() - 5);
            var timeStr = date.toString();

            diaglog.reset();
            log = diaglog.search().keypattern( 'rc: ' ).timeEnd( timeStr );
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
            testWithoutOriginal(diaglog);

            // 排除文件名影响，对比从集群日志搜索的结果是否和从 collect() 收集回来的文件的结果一致
            cmd.run( 'sed -i "s#,/[^,]*,#,#g" ' + fileName1 );
            cmd.run( 'sed -i "s#,/[^,]*,#,#g" ' + fileName2 );
            rc = cmd.run( 'diff -q ' + fileName1 + ' ' + fileName2 + ' > /dev/null 2>&1; echo $?' ).trimRight( '\n' );
            assert.equal( rc, '0' );

            log = diaglog.analyze();
            fileName = log.run();
            testCsv(fileName);

            File.remove( WORKDIR + '/diaglog_34330' );
        } catch ( e ) {
            println('filename: ' + fileName);
            println('filename1: ' + fileName1);
            println('filename2: ' + fileName2);
            println('packageName:' + packageName);
            println('diaglog: ' + JSON.stringify(diaglog));
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

function testWithoutOriginal( diaglog )
{
    // 检查简要模式下查询的结果是否按时间降序，以及各字段内容是否正常有值
    try {
        var line = "";
        var preTime = "999999999999999999999999999";
        var curTime = "";
        var array = [];
        var rc;
        var lastArray = [];
        while ( line = diaglog.next() )
        {
            lastArray = array;
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
        println('lastArray: ' + JSON.stringify(lastArray));
        println('preTime: ' + preTime);
        throw e ;
    }
}