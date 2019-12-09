/****************************************************
@description:	query, normal case
         testlink cases: seqDB-7203
@input:		insert 6 recs
            queryAndCheck(): cover all parameters
@expectation:	1 "cmd=query&name=local_test_cs.local_test_cl&sort={"age":1}&skip=1&returnnum=3&filter={"male":true}"
				2 varCL.find({male:true}).skip(1).limit(3).sort({"age":1})
				3 step 1 result == step 2 result
@modify list:
            	2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME;

function insertRecs ()
{
	try
	{
		varCL.insert( [
			{ age: 12, name: "Tom", male: true },
			{ age: 13, name: "Anna", male: false },
			{ age: 15, name: "Jack", male: true },
			{ age: 11, name: "Harry", male: true },
			{ age: 19, name: "Bob", male: true },
			{ age: 9, name: "Jobs", male: true }
		] );
	} catch( e )
	{
		println( "fail to insert records in begin" );
		throw e;
	}
}

function queryAndCheck ()
{
	var word = "query";
	tryCatch(
		["cmd=" + word, "name=" + csName + '.' + clName, 'sort={"age":1}', 'skip=1', 'selector={"age":""}', 'returnnum=3', 'filter={"male":true}'],
		[0],
		"queryAndCheck: fail to run rest command: " + word );

	var arr = infoSplit;
	var rc = varCL.find( { male: true }, { age: { $include: 1 } } ).skip( 1 ).limit( 3 ).sort( { "age": 1 } );
	var i = 1;
	try
	{
		while( rc.next() )
		{
			if( rc.current().toObj()["name"] !== JSON.parse( arr[i] ).name )
			{
				println( "rest command: " + str + "\nrest return: " + info );
				println( "the " + i + " record, cl.find().current().name: " + rc.current().toObj()["name"] + ", rest return: " + JSON.parse( arr[i] ).name );
				throw "rest cmd=" + word + " result is different from rc.find() ";
			}
			i++;
		}
	} catch( e )
	{
		throw e;
	}
}

commDropCL( db, csName, clName, true, true, "drop cl in begin" );
var opt = { ReplSize: 0 };
var varCL = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );
insertRecs();
queryAndCheck();
commDropCL( db, csName, clName, false, false, "drop cl in clean" );