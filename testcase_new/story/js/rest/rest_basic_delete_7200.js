/****************************************************
@description:	delete, normal case
         testlink cases: seqDB-7200 & 7201
@input:		1 insert 6 recs
				2 "cmd=delete&name=local_test_cs.local_test_cl&deletor={"male":true}"
				3 lack [deletor], the deletor is an optional parameters.
@expectation:	db.find(), return only 1 rec, [name] field of the rec is "Anna"		
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
			{ age: 11, name: "Tom", male: true },
			{ age: 12, name: "Anna", male: false },
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

function deleteAndCheck ()
{
	var word = "delete";
	tryCatch(
		["cmd=" + word, "name=" + csName + '.' + clName, 'deletor={male:true}'],
		[0],
		"deleteAndCheck: fail to run rest command: " + word );
	var num = 1;
	var arr = infoSplit;
	var rc = varCL.find();
	try
	{
		var size = 0;
		while( rc.next() )
		{
			size++;
			if( size != num ) { throw "rest cmd=" + word + " has been done, expect: cl.find()=1, actual: >1"; }
			if( rc.current().toObj()["name"] != "Anna" )
			{
				throw "rest cmd=" + word + " has been done, expect: cl.find()[Name]='Anna', actual: " + rc.current().toObj()["name"];
			}
		}
	} catch( e )
	{
		throw e;
	}
}

function lackDeletor ()
{  //The deletor is an optional parameter.
	var word = "delete";
	tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName],
		[0], "deleteAndCheck: fail to run rest command: " + word );

	/**********check result***************/
	try
	{
		var size = varCL.count();
		if( 0 != size )
		{
			throw "rest [cmd=delete] failed, actual size: " + size;
		}
	}
	catch( e )
	{
		throw e;
	}
}

commDropCL( db, csName, clName, true, true, "drop cl in begin" );
var opt = { ReplSize: 0 };
var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
insertRecs();
deleteAndCheck();
lackDeletor();
commDropCL( db, csName, clName, false, false, "drop cl in clean" );