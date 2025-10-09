/************************************
*@Description: seqDB-34328 collect 功能基本参数测试
*@author:      jiangqiqian
*@createDate:  2025.09.15
**************************************/

main( test );

function testCore( diaglog, db, coreFileNameArray )
{
    var log;
    var fileName;
    try {
        var cursor = db.exec( 'select HostName,ServiceName,GroupName,Disk from $SNAPSHOT_SYSTEM limit 3' );
        while ( cursor.next() ) {
            var current = cursor.current().toObj();
            var HostName = current.HostName;
            var ServiceName = current.ServiceName;
            var GroupName = current.GroupName;
            var DatabasePath = current.Disk.DatabasePath;
            coreFileNameArray.push( HostName + '_' + ServiceName + '_' + GroupName + '_diaglog_test.core');
            var remote = new Remote( HostName, CMSVCNAME );
            var cmd = remote.getCmd();
            // 构造 core 文件
            cmd.run( 'echo "diaglog test collect core" > ' + DatabasePath + '/diaglog/diaglog_test.core' );
            remote.close();
        }
        cursor.close();
    } catch ( e ) {
        throw e;
    } finally {
        if ( null != remote ){
            remote.close();
        }
        if ( null != cursor ){
            cursor.close();
        }
    }

    try {
        diaglog.reset();
        log = diaglog.collect().core();
        fileName = log.run();
    } catch ( e ) {
        throw e;
    }

    try {
        // 检查 core 文件是否被收集
        for ( let i = 0; i < coreFileNameArray.length; i++ ) {
            var rc = File.exist(  fileName + '/trap_core_snapshot/' + coreFileNameArray[i] );
            assert.equal( rc, true );
        }
    } catch ( e ) {
        throw e;
    }
    diaglog.reset();
}

function testTrap( diaglog, db, trapFileNameArray )
{
    var log;
    var fileName;
    try {
        var cursor = db.exec( 'select HostName,ServiceName,GroupName,Disk from $SNAPSHOT_SYSTEM limit 3' );
        while ( cursor.next() ) {
            var current = cursor.current().toObj();
            var HostName = current.HostName;
            var ServiceName = current.ServiceName;
            var GroupName = current.GroupName;
            var DatabasePath = current.Disk.DatabasePath;
            trapFileNameArray.push( HostName + '_' + ServiceName + '_' + GroupName + '_diaglog_test.trap');
            var remote = new Remote( HostName, CMSVCNAME );
            var cmd = remote.getCmd();
            // 构造 trap 文件
            cmd.run( 'echo "diaglog test collect trap" > ' + DatabasePath + '/diaglog/diaglog_test.trap' );
            remote.close();
        }
        cursor.close();
    } catch ( e ) {
        throw e;
    } finally {
        if ( null != remote ){
            remote.close();
        }
        if ( null != cursor ){
            cursor.close();
        }
    }

    try {
        diaglog.reset();
        log = diaglog.collect().trap();
        fileName = log.run();
    } catch ( e ) {
        throw e;
    }

    try {
        // 检查 trap 文件是否被收集
        for ( let i = 0; i < trapFileNameArray.length; i++ ) {
            var rc = File.exist(  fileName + '/trap_core_snapshot/' + trapFileNameArray[i] );
            assert.equal( rc, true );
        }
    } catch ( e ) {
        throw e;
    }
    diaglog.reset();
}

function testSnapShot( diaglog, localCmd )
{
    // 快照列表
    // SNAP_CSCL: $SNAPSHOT_CS $SNAPSHOT_CL $SNAPSHOT_CATA
    // SNAP_SYS: $SNAPSHOT_SYSTEM $SNAPSHOT_CONFIGS $SNAPSHOT_DB $SNAPSHOT_HEALTH $SNAPSHOT_SEQUENCES $SNAPSHOT_SVCTASKS $SNAPSHOT_TASKS
    // SNAP_SESSION: $SNAPSHOT_SESSION $SNAPSHOT_CONTEXT
    // SNAP_QUERY: $SNAPSHOT_QUERIES $SNAPSHOT_LOCKWAITS $SNAPSHOT_LATCHWAITS $SNAPSHOT_TRANS $SNAPSHOT_ACCESSPLANS $SNAPSHOT_INDEXSTATS $SNAPSHOT_TRANSDEADLOCK $SNAPSHOT_TRANSWAIT
    // SNAP_ALL: 上面全部
    var rc;
    var log;
    var fileName;
    try {
        diaglog.reset();
        log = diaglog.collect().snapshot( 'SNAP_CSCL' );
        fileName = log.run();
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_cs' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_cl' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_cata' );
        assert.equal( rc, true );

        log = diaglog.collect().snapshot( 'SNAP_SYS' );
        fileName = log.run();
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_system' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_configs' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_db' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_health' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_sequences' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_svctasks' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_tasks' );
        assert.equal( rc, true );

        log = diaglog.collect().snapshot( 'SNAP_SESSION' );
        fileName = log.run();
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_session' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_context' );
        assert.equal( rc, true );

        log = diaglog.collect().snapshot( 'SNAP_QUERY' );
        fileName = log.run();
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_queries' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_lockwaits' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_latchwaits' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_trans' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_accessplans' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_indexstats' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_transwait' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_transdeadlock' );
        assert.equal( rc, true );

        log = diaglog.collect().snapshot( 'SNAP_ALL' );
        fileName = log.run();
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_cs' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_cl' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_cata' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_system' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_configs' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_db' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_health' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_sequences' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_svctasks' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_tasks' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_session' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_context' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_queries' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_lockwaits' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_latchwaits' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_trans' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_accessplans' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_indexstats' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_transwait' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_transdeadlock' );
        assert.equal( rc, true );
    } catch ( e ) {
        throw e;
    }
    diaglog.reset();
}

function testAll( diaglog, coreFileNameArray, trapFileNameArray )
{
    var rc;
    var log;
    var fileName;

    try {
        diaglog.reset();
        log = diaglog.collect().all();
        fileName = log.run();

        // 检查 core 文件是否被收集
        for ( let i = 0; i < coreFileNameArray.length; i++ ) {
            rc = File.exist(  fileName + '/trap_core_snapshot/' + coreFileNameArray[i] );
            assert.equal( rc, true );
        }

        // 检查 trap 文件是否被收集
        for ( let i = 0; i < trapFileNameArray.length; i++ ) {
            rc = File.exist(  fileName + '/trap_core_snapshot/' + trapFileNameArray[i] );
            assert.equal( rc, true );
        }

        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_cs' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_cl' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_cata' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_system' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_configs' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_db' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_health' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_sequences' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_svctasks' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_tasks' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_session' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_context' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_queries' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_lockwaits' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_latchwaits' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_trans' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_accessplans' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_indexstats' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_transwait' );
        assert.equal( rc, true );
        rc = File.exist( fileName + '/trap_core_snapshot/snapshot_transdeadlock' );
        assert.equal( rc, true );
    } catch ( e ) {
        throw e;
    }
    diaglog.reset();
}

function testCompress( diaglog ) {
    var log;
    var fileName;
    var rc;
    try {
        diaglog.reset();
        log = diaglog.collect().trap().path( WORKDIR + '/diaglog_34328' );
        fileName = log.run();
        // 检查压缩格式是否为 tar.gz
        rc = File.exist(  fileName + '.tar.gz' );
        assert.equal( rc, true );
        File.remove( fileName );
        File.remove( fileName + '.tar.gz' );

        log = diaglog.collect().trap().path( WORKDIR + '/diaglog_34328' ).compress( 'tar.gz' );
        fileName = log.run();
        // 检查压缩格式是否为 tar.gz
        rc = File.exist(  fileName + '.tar.gz' );
        assert.equal( rc, true );
        File.remove( fileName );
        File.remove( fileName + '.tar.gz' );

        log = diaglog.collect().trap().path( WORKDIR + '/diaglog_34328' ).compress( 'zip' );
        fileName = log.run();
        // 检查压缩格式是否为 zip
        rc = File.exist(  fileName + '.zip' );
        assert.equal( rc, true );
        File.remove( fileName );
        File.remove( fileName + '.zip' );
    } catch ( e ) {
        throw e;
    }

    // 非 tar.gz / zip 报错
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        diaglog.collect().trap().path( WORKDIR + '/diaglog_34328' ).compress( 'abc' );
    } );
    diaglog.reset();
}

function testPath( diaglog )
{
    var log;
    var fileName;
    var rc;

    // 不指定，预期成功，自动生成目录
    try {
        diaglog.reset();
        log = diaglog.collect().trap();
        fileName = log.run();
        log = diaglog.collect().trap();
        fileName = log.run();
        log = diaglog.collect().trap();
        fileName = log.run();
        log = diaglog.collect().trap();
        fileName = log.run();
        log = diaglog.collect().trap();
        fileName = log.run();
    } catch ( e ) {
        throw e;
    }

    try {
        var cmd = new Cmd();
        // 前面多次执行未指定输出目录，此时临时目录下应存在 10 个目录（上限）
        rc = cmd.run( 'ls -d /tmp/sequoiadb/collect/diaglog_*.auto | wc -l' ).trimRight( '\n' );
        assert.equal( rc, '10' );
        rc = cmd.run( 'ls /tmp/sequoiadb/collect/diaglog_*.auto.tar.gz | wc -l' ).trimRight( '\n' );
        assert.equal( rc, '10' );
    } catch ( e ) {
        throw e;
    }

    // 指定不存在的目录，预期成功
    try {
        log = diaglog.collect().error( -16 ).limit( 1 ).path( WORKDIR + '/diaglog_34328/abcde' );
        fileName = log.run();
        rc = File.exist( WORKDIR + '/diaglog_34328/abcde' );
        assert.equal( rc, true );
    } catch ( e ) {
        throw e;
    }

    // 指定非字符串，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.collect().error( -16 ).limit( 1 ).path( 123 );
    } );

    // 指定非绝对路径，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.collect().error( -16 ).limit( 1 ).path( './diaglog_1/result_12' );
    } );

    // 指定空字符串，预期失败
    assert.tryThrow( SDB_INVALIDARG, function()
    {
        log = diaglog.collect().error( -16 ).limit( 1 ).path( '' );
    } );
    diaglog.reset();
}

function test()
{
    try {
        var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
        var diaglog = new DiagLog( COORDHOSTNAME, COORDSVCNAME );
        var coreFileNameArray = [];
        var trapFileNameArray = [];

        // collect core
        testCore( diaglog, db, coreFileNameArray );
    
        // collect trap
        testTrap( diaglog, db, trapFileNameArray );

        // collect snasphot
        testSnapShot( diaglog );
    
        // collect core, trap, snapshot
        testAll( diaglog, coreFileNameArray, trapFileNameArray );

        // compress
        testCompress( diaglog );

        // path
        testPath( diaglog );

        File.remove( WORKDIR + '/diaglog_34328' );
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