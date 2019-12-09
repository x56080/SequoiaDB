/****************************************************
@description:	get count, normal case
         testlink cases: seqDB-7218
@input:		insert 1 rec
@expectation:	varCL.count() result == get count by rest
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
			{ age: 11, name: "Tom", male: true }
		] );
	} catch( e )
	{
		println( "fail to insert records in begin" );
		throw e;
	}
}
function getcountAndCheck ()
{
	var word = "get count";
	tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName], [0] );
	var arr = infoSplit;

	try
	{
		if( JSON.parse( arr[1] ).Total != varCL.count() )
		{
			throw "rest command  " + word + "result is " + JSON.parse( arr[1] ).Total + ", but cl.count() result is " + 1;
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
getcountAndCheck();
commDropCL( db, csName, clName, false, false, "drop cl in clean" );
