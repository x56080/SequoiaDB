/************************************
*@Description: 指定中间记录执行先逆向pop后正向pop
*@author:      liuxiaoxuan
*@createdate:  2017.9.30
*@testlinkCase: seqDB-12806
**************************************/
function main ()
{
	var clName = COMMCAPPEDCLNAME + "_12806";
	var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
	var dbcl = commCreateCLByOption( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

	//insert records 
	var insertNums = 32768;
	insertData( dbcl, insertNums );

	//get one of record in the middle
	var mid_position = 20000;
	var orderby = 1
	var logicalID = getMiddleOneId( dbcl, mid_position, orderby );

	//pop with direction -1 
	var direction = -1;
	popLogicalID( dbcl, logicalID, direction );

	//check count
	var expectCount = 20000;
	checkCount( dbcl, expectCount );

	//insert records again
	insertData( dbcl, insertNums );
	//check count
	expectCount = expectCount + insertNums;
	checkCount( dbcl, expectCount );

	//get one of record in the middle
	mid_position = 25000;
	orderby = -1;
	logicalID = getMiddleOneId( dbcl, mid_position, orderby );

	//pop with direction 1 
	direction = 1;
	popLogicalID( dbcl, logicalID, direction );

	//check count 
	expectCount = 25000;
	checkCount( dbcl, expectCount );

	commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}
main();

function insertData ( dbcl, insertNums )
{
	var str = "testaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaatesttesttetestaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	var doc = [];
	for( var i = 0; i < 32768; i++ )
	{
		doc.push( { a: i, b: str } );
	}
	try
	{
		dbcl.insert( doc );
	}
	catch( e )
	{
		throw buildException( "insert datas", e, "insert data", "insert success", "insert fail" );
	}
}

function getMiddleOneId ( dbcl, position, order )
{
	var logicalID = -1;
	try
	{
		var rec = dbcl.find().sort( { _id: order } ).skip( position ).limit( 1 );
		logicalID = rec.current().toObj()._id;
	}
	catch( e )
	{
		throw buildException( "getMiddleOneId()", e, "getMiddleOneId", "get lid success", "get lid fail" );
	}
	return logicalID;
}

function popLogicalID ( dbcl, logicalID, direction )
{
	try
	{
		dbcl.pop( { LogicalID: logicalID, Direction: direction } );
	}
	catch( e )
	{
		throw buildException( "popLogicalID", e, "pop", "pop success", "pop fail" );
	}
}

function checkCount ( dbcl, expectCount )
{
	try
	{
		var actCount = dbcl.count();
		if( expectCount != actCount )
		{
			throw "CHECK COUNT FAIL";
		}
	}
	catch( e )
	{
		throw buildException( "checkCount()", e, "check count", "check success", "check fail" );
	}
}