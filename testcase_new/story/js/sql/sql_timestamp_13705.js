/************************************
*@Description: 内置SQL支持timestamp带条件查询 
*@author:      wangkexin
*@createdate:  2019.3.2
*@testlinkCase:seqDB-13705
**************************************/

main();

function main ()
{
    var csName = COMMCSNAME;
    var clName = "cl13705";

    var cl = commCreateCL( db, csName, clName, null, null, true, false, "create cl in the begin" );

    //正常timestamp类型数据、边界值、非法值
    //Timestamp类型能表示的时间范围为1902-01-01 00:00:00.000000至2037-12-31 23:59:59.999999
    insertSQL( db, cl, csName, clName, 'Timestamp("2019-03-02-10:48:50.000000")', { $timestamp: "2019-03-02-10.48.50.000000" }, true );
    var timestamp = insertSQL( db, cl, csName, clName, 'Timestamp("1902-01-01-00:00:00.000000")', { $timestamp: "1902-01-01-00.00.00.000000" }, true );
    insertSQL( db, cl, csName, clName, 'Timestamp("2037-12-31-23:59:59.999999")', { $timestamp: "2037-12-31-23.59.59.999999" }, true );
    insertSQL( db, cl, csName, clName, 'Timestamp("1800-01-01-00:00:00.000000")', false );
    insertSQL( db, cl, csName, clName, 'Timestamp("2038-12-20-00:00:00.000000")', false );
    insertSQL( db, cl, csName, clName, 'Timestamp("978192000000")', false );

    selectSQL( db, csName, clName, 'Timestamp("2019-03-02-10:48:50.000000")', { $timestamp: "2019-03-02-10.48.50.000000" }, true );
    selectSQL( db, csName, clName, 'Timestamp("1902-01-01-00:00:00.000000")', { $timestamp: timestamp }, true );
    selectSQL( db, csName, clName, 'Timestamp("2037-12-31-23:59:59.999999")', { $timestamp: "2037-12-31-23.59.59.999999" }, true );
    selectSQL( db, csName, clName, 'Timestamp("1800-01-01-00:00:00.000000")', false );
    selectSQL( db, csName, clName, 'Timestamp("2038-12-20-00:00:00.000000")', false );
    selectSQL( db, csName, clName, 'Timestamp("978192000000")', false );

    updateSQL( db, cl, csName, clName, 'Timestamp("2019-03-02-10:48:50.000000")', 'Timestamp("2019-03-01-10:48:50.000000")', { $timestamp: "2019-03-01-10.48.50.000000" }, true );
    updateSQL( db, cl, csName, clName, 'Timestamp("1902-01-01-00:00:00.000000")', 'Timestamp("1902-12-01-00:00:00.000000")', { $timestamp: "1902-12-01-00.00.00.000000" }, true );
    updateSQL( db, cl, csName, clName, 'Timestamp("2037-12-31-23:59:59.999999")', 'Timestamp("2036-12-31-23.59.59.999999")', { $timestamp: "2036-12-31-23.59.59.999999" }, true );
    updateSQL( db, cl, csName, clName, 'Timestamp("1800-01-01-00:00:00.000000")', 'Timestamp("1800-12-01-00.00.00.000000")', false );
    updateSQL( db, cl, csName, clName, 'Timestamp("2038-12-20-00:00:00.000000")', 'Timestamp("2038-12-30-00:00:00.000000")', false );
    updateSQL( db, cl, csName, clName, 'Timestamp("978192000000")', 'Timestamp("978192000000")', false );

    deleteSQL( db, cl, csName, clName, 'Timestamp("2019-03-01-10:48:50.000000")', { $timestamp: "2019-03-01-10.48.50.000000" }, true );
    deleteSQL( db, cl, csName, clName, 'Timestamp("1902-12-01-00:00:00.000000")', { $timestamp: "1902-12-01-00.00.00.000000" }, true );
    deleteSQL( db, cl, csName, clName, 'Timestamp("2036-12-31-23:59:59.999999")', { $timestamp: "2036-12-31-23.59.59.999999" }, true );
    deleteSQL( db, cl, csName, clName, 'Timestamp("1800-01-01-00:00:00.000000")', false );
    deleteSQL( db, cl, csName, clName, 'Timestamp("2038-12-20-00:00:00.000000")', false );
    deleteSQL( db, cl, csName, clName, 'Timestamp("978192000000")', false );

    if( cl.count() != 0 )
    {
        throw buildException( "main()", null, "check cl data", "no data", "have data" );
    }
    commDropCL( db, csName, clName, true, true, "drop CL in the end" );
}

function insertSQL ( db, cl, csName, clName, insertValue, checkValue, result )
{
    var sql = 'insert into ' + csName + '.' + clName + '(num, textFields ) values (3, ' + insertValue + ')';
    if( result )
    {
        var cursor = null;
        try
        {
            db.execUpdate( sql );
            if( JSON.stringify( checkValue ) == '{"$timestamp":"1902-01-01-00.00.00.000000"}' )
            {
                // 在1928年1月1日前，由UTC时间戳转成中国本地时区的时间，会加上5分52秒，为规避此问题，这里使用三个时间戳进行匹配，避免不同机器上产生的误差
                var checkValue1 = { $timestamp: "1901-12-31-23.54.08.000000" };
                var checkValue2 = { $timestamp: "1902-01-01-00.05.52.000000" };
                var count = cl.find( { textFields: { $in: [checkValue1, checkValue, checkValue2] } } ).count();
                if( Number( count ) !== 1 )
                {
                    throw buildException( "insertSQL()", null, "check record " + insertValue, 1, Number( count ) );
                }
                var cursor = cl.find( { textFields: { $in: [checkValue1, checkValue, checkValue2] } } );
                var timestamp = cursor.next().toObj()["textFields"]["$timestamp"];
                return timestamp;
            }
            else
            {
                cursor = cl.find( { textFields: checkValue }, { "_id": { "$include": 0 } } );
                var expstring = '{"num":3,"textFields":' + JSON.stringify( checkValue ) + '}';
                var actString = JSON.stringify( cursor.next().toObj() );
                if( expstring !== actString )
                {
                    throw buildException( "insertSQL()", null, "check record " + insertValue, expstring, actString );
                }
            }

        }
        catch( e )
        {
            throw buildException( "insertSQL()", e, "insert record " + insertValue, "insert success", "insert failed: " + e );
        }
        finally
        {
            if( cursor != null )
            {
                cursor.close();
            }
        }
    }
    else
    {
        try
        {
            db.execUpdate( sql );
            throw buildException( "insertSQL()", null, "insert error record " + insertValue, "insert failed", "insert success" );
        }
        catch( e )
        {
            if( e != -6 && e != -195 )
            {
                throw buildException( "insertSQL()", e, "insert record " + insertValue, '-6', e );
            }
        }
    }
}

function updateSQL ( db, cl, csName, clName, oldValue, newValue, checkValue, result )
{
    var sql = 'update ' + csName + '.' + clName + ' set textFields=' + newValue + ' where textFields=' + oldValue;
    if( result )
    {
        var cursor = null;
        try
        {
            db.execUpdate( sql );
            if( JSON.stringify( checkValue ) == '{"$timestamp":"1902-12-01-00.00.00.000000"}' )
            {
                // 在1928年1月1日前，由UTC时间戳转成中国本地时区的时间，会加上5分52秒，为规避此问题，这里使用三个时间戳进行匹配，避免不同机器上产生的误差
                var checkValue1 = { $timestamp: "1902-11-30-23.54.08.000000" };
                var checkValue2 = { $timestamp: "1902-12-01-00.05.52.000000" };
                var count = cl.find( { textFields: { $in: [checkValue1, checkValue, checkValue2] } } ).count();
                if( Number( count ) !== 1 )
                {
                    throw buildException( "updateSQL()", null, "check record " + oldValue, 1, Number( count ) );
                }
            }
            else
            {
                cursor = cl.find( { textFields: checkValue }, { "_id": { "$include": 0 } } );
                var expstring = '{"num":3,"textFields":' + JSON.stringify( checkValue ) + '}';
                var actString = JSON.stringify( cursor.next().toObj() );
                if( expstring !== actString )
                {
                    throw buildException( "updateSQL()", null, "check record " + oldValue, expstring, actString );
                }
            }
        }
        catch( e )
        {
            throw buildException( "updateSQL()", e, "update record " + oldValue + " to " + newValue, "update success", "update failed: " + e );
        }
        finally
        {
            if( cursor != null )
            {
                cursor.close();
            }
        }
    }
    else
    {
        try
        {
            db.execUpdate( sql );
            throw buildException( "updateSQL()", null, "update record " + oldValue + " to " + newValue + " should failed", "update failed", "update success" );
        }
        catch( e )
        {
            if( e != -6 && e != -195 )
            {
                throw buildException( "updateSQL()", e, "update record " + oldValue + " to " + newValue, '-6', e );
            }
        }
    }
}

function selectSQL ( db, csName, clName, value, checkValue, result )
{
    var sql = 'select num,textFields from ' + csName + "." + clName + ' where textFields=' + value;
    if( result )
    {
        try
        {
            var cursor = db.exec( sql );
            var expstring = '{"num":3,"textFields":' + JSON.stringify( checkValue ) + '}';
            var actString = JSON.stringify( cursor.next().toObj() );
            if( expstring !== actString )
            {
                throw buildException( "selectSQL()", null, "check record " + value, expstring, actString );
            }
        }
        catch( e )
        {
            throw buildException( "selectSQL()", e, "select record " + value, "select success", "select failed: " + e );
        }
        finally
        {
            cursor.close()
        }
    }
    else
    {
        try
        {
            db.exec( sql );
            throw buildException( "selectSQL()", null, "select error record " + value, "select failed", "select success" );
        }
        catch( e )
        {
            if( e != -6 && e != -195 )
            {
                throw buildException( "selectSQL()", e, "select record " + value, '-6', e );
            }
        }
    }
}

function deleteSQL ( db, cl, csName, clName, deleteValue, checkValue, result )
{
    var sql = 'delete from ' + csName + '.' + clName + ' where textFields=' + deleteValue;
    if( result )
    {
        try
        {
            db.execUpdate( sql );
            var cursor = cl.find( { textFields: checkValue } );
            if( cursor.next() != null )
            {
                throw buildException( "deleteSQL()", null, "check record " + deleteValue, "no data", "have data" );
            }
        }
        catch( e )
        {
            throw buildException( "deleteSQL()", e, "delete record " + deleteValue, "delete success", "delete failed: " + e );
        }
        finally
        {
            cursor.close()
        }
    }
    else
    {
        try
        {
            db.execUpdate( sql );
            throw buildException( "deleteSQL()", null, "delete error record " + deleteValue, "delete failed", "delete success" );
        }
        catch( e )
        {
            if( e != -6 && e != -195 )
            {
                throw buildException( "deleteSQL()", e, "delete record " + deleteValue, '-6', e );
            }
        }
    }
}