import( "../lib/main.js" );

var allTypeData = [ 2147483646, 9223372036854775806, 1.7E+30, { "$decimal": "123.456" }, "String", { "$oid" : "123abcd00ef12358902300ef" }, true, { "$date": "2012-01-01" }, { "$timestamp": "2012-01-01-13.14.26.124233" }, { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, { "$regex": "^张", "$options": "i" }, [ 1, "string" ], null, { "$minKey": 1 }, { "$maxKey": 1 }, { "parent": { "child": 1 } }, [ [ "child1", "child11" ], [ "child2", "child22" ] ], [ { "key1": "value1" }, { "key2": "value2" } ] ];

/************************************
*@Description: insert data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function insertData ( dbcl, condition )
{
    try
    {
        dbcl.insert( condition );
    } catch( e )
    {
        throw new Error( e );
    }
}

/************************************
*@Description: update data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function updateData ( dbcl, updateCondition, findCondition )
{
    if( typeof ( findCondition ) == "undefined" ) { findCondition = null; }
    try
    {
        dbcl.update( updateCondition, findCondition );
    } catch( e )
    {
        throw new Error( e );
    }
}

/************************************
*@Description: upsert data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function upsertData ( dbcl, upsertCondition, findCondition )
{
    if( typeof ( findCondition ) == "undefined" ) { findCondition = null; }
    try
    {
        dbcl.upsert( upsertCondition, findCondition );
    } catch( e )
    {
        throw new Error( e );
    }
}

/************************************
*@Description: delete data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function deleteData ( dbcl, condition )
{
    if( typeof ( condition ) == "undefined" ) { condition = null; }
    try
    {
        dbcl.remove( condition );
    } catch( e )
    {
        throw new Error( e );
    }
}

/************************************
*@Description: find and sort data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function sortFindData ( dbcl, findCondition, findCondition2, sortCondition )
{
    if( typeof ( findCondition ) == "undefined" ) { findCondition = null; }
    try
    {
        var sortResult = dbcl.find( findCondition, findCondition2 ).sort( sortCondition );
    } catch( e )
    {
        throw new Error( e );
    }
    return sortResult;
}

/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkResult ( dbcl, findCondition, findCondition2, expRecs, sortCondition )
{
    var rc = sortFindData( dbcl, findCondition, findCondition2, sortCondition );
    checkRec( rc, expRecs );
}

/************************************
*@Description: compare actual and expect result,
               they is not the same ,return error ,
               else return ok
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkRec ( rc, expRecs )
{
    //get actual records to array
    var actRecs = [];
    while( rc.next() )
    {
        actRecs.push( rc.current().toObj() );
    }
    //check count
    if( actRecs.length !== expRecs.length )
    {
        throw new Error( "expect num: " + expRecs.length + ",actual num: " + actRecs.length
            + "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
    }

    //check every records every fields,expRecs as compare source
    for( var i in expRecs )
    {
        var actRec = actRecs[i];
        var expRec = expRecs[i];

        for( var f in expRec )
        {
            if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
            {
                throw new Error( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'"
                    + "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
            }
        }
    }
    //check every records every fields,actRecs as compare source
    for( var i in actRecs )
    {
        var actRec = actRecs[i];
        var expRec = expRecs[i];

        for( var f in actRec )
        {
            if( f == "_id" )
            {
                continue;
            }
            if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
            {
                throw new Error( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'"
                    + "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
            }
        }
    }
}

/************************************
*@Description: check result when the expect result of update data is failed.
*@author:      zhaoyu 
*@createDate:  2016/5/16
*@parameters:               
**************************************/
function invalidDataUpdateCheckResult ( dbcl, invalidDoc, expRecs )
{
    try
    {
        dbcl.update( invalidDoc );
        throw new Error( "need throw error" );
    } catch( e )
    {
        if( expRecs != e )
        {
            throw new Error( "expect error: " + expRecs + "actual error : " + e );
        }
    }
}

/************************************
*@Description: get group name and service name .
*@author:      wuyan 
*@createDate:  2015/10/20
*@parameters:               
**************************************/
function getGroupName ( db, mustBePrimary )
{
    var RGname = null;
    try
    {
        RGname = db.listReplicaGroups().toArray();
        var j = 0;
        var arrGroupName = Array();
        for( var i = 1; i != RGname.length; ++i )
        {
            var eRGname = eval( '(' + RGname[i] + ')' );
            if( 1000 <= eRGname["GroupID"] )
            {
                arrGroupName[j] = Array();
                var primaryNodeID = eRGname["PrimaryNode"];
                var groups = eRGname["Group"];
                for( var m = 0; m < groups.length; m++ )
                {
                    if( true == mustBePrimary )
                    {
                        var nodeID = groups[m]["NodeID"];
                        if( primaryNodeID != nodeID )
                            continue;
                    }
                    arrGroupName[j].push( eRGname["GroupName"] );
                    arrGroupName[j].push( groups[m]["HostName"] );
                    arrGroupName[j].push( groups[m]["Service"][0]["Name"] );
                    break;
                }
                ++j;
            }
        }
    } catch( e )
    {
        throw new Error( e );
    }

    return arrGroupName;
}

/************************************
*@Description: create index
*@author:      liuxiaoxuan
*@createDate:  2017.09.18
**************************************/
function createIndex ( dbcl, indexName, key )
{
    try
    {
        dbcl.createIndex( indexName, key );
    } catch( e )
    {
        throw new Error( e );
    }
}
