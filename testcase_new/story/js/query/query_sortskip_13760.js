/*******************************************************************************
*@Description : test query by use sort and skip function. [jira_503].
*               such as: db.foo.bar.find().sort({a:1}).skip(101)
*@Modify list :
*               2014-11-10  xiaojun Hu  Init
*               2019-08-23  wangkexin   Modified
*******************************************************************************/
main()
function main ()
{
    var clName = "cl13760";
    var insertNum = 50;

    var cl = readyCL( clName );
    // insert data
    var records = readyData( cl, insertNum );
    println( "success to insert data, number = " + insertNum );
    // query by using sort and skip function
    var skipNum = insertNum / 2;
    testSortAndSkip( cl, records, 1, skipNum );
    testSortAndSkip( cl, records, -1, skipNum );
    println( "success to query by using sort and skip function when don't have index" );

    // create Index
    var idxName = "noIndex13760";
    var indexDef = { "a": 1 };
    commCreateIndex( cl, idxName, indexDef );
    commCheckIndexConsistency( cl, idxName, true );
    println( "create index successful" );

    // query by using sort and skip function. and create index
    var skipNum = insertNum / 3;
    testSortAndSkip( cl, records, 1, skipNum );
    testSortAndSkip( cl, records, -1, skipNum );
    println( "success to query by using sort and skip function while having index" );
    commDropCL( db, COMMCSNAME, clName, false, false, "clear collection in the end." );
}

function readyData ( cl, insertNum )
{
    try
    {
        var orderedRecords = new Array();
        println( "\n---Begin to insert cl data." );
        var records = new Array();
        for( var i = 0; i < insertNum; i++ )
        {
            records[i] = { _id: i, a: i, str: "strTest13760_" + i };
        }
        //将有序的数组中数据保存在orderedRecords并返回（使用slice()避免将原数组的引用赋值给orderedRecords）
        var orderedRecords = records.slice();
        //将有序的数组中数据打乱顺序并插入到集合中
        var randomRecords = records.sort( function() { return 0.5 - Math.random() } );
        cl.insert( randomRecords );
        return orderedRecords;
    }
    catch( e )
    {
        throw buildException( "readyData()", e, "insert", "insert success", "insert fail" );
    }
}

function testSortAndSkip ( cl, records, sortOrder, skipNum )
{
    var tmpRecords = records.slice();
    if( sortOrder == -1 )
    {
        tmpRecords.reverse();
    }
    var rc = cl.find().sort( { a: sortOrder } ).skip( skipNum );
    var expRecords = tmpRecords.slice( skipNum );
    checkRec( rc, expRecords );
}
