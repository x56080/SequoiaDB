/************************************
*@Description: connect data group to drop CL
*@author:      liuxiaoxuan
*@createdate:  2017.10.09
*@testlinkCase: seqDB-12808
**************************************/
function main ()
{
	if( commIsStandalone( db ) )
	{
		println( 'standlone' );
		return;
	}

	var clName = COMMCAPPEDCLNAME + "12808";
	var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
	var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, true, true );

	//get primary node
	var dataGroup = commGetCLGroups( db, COMMCAPPEDCSNAME + "." + clName );

	var priNodeAddr = getMasterNodeName( dataGroup[0] );

	//connect to primary node
	var dataDB = getDataDB( priNodeAddr );
	println( 'dataDB: ' + dataDB );
	//drop cl at priNode
	commDropCL( dataDB, COMMCAPPEDCSNAME, clName, 'drop CL' );

	//insert data with coord
	var doc = [];
	var insertNums = 10;
	for( var i = 0; i < insertNums; i++ )
	{
		doc.push( { a: i } );
	}
	insertDatas( dbcl, doc );

	//check cl
	checkCappedCL( dataDB, COMMCAPPEDCSNAME, clName );

	commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function getMasterNodeName ( groupName )
{
	println( 'group: ' + groupName );
	var priNode = null;
	try
	{
		var rg = db.getRG( groupName );
		priNode = rg.getMaster();
	}
	catch( e )
	{
		throw buildException( "getMasterNodeName()", e, "get master node", "success", "fail:" + e );
	}
	return priNode;
}

function getDataDB ( nodeAddr )
{
	var dataDB = null;
	try
	{
		dataDB = new Sdb( nodeAddr );
	}
	catch( e )
	{
		throw buildException( null, null,
			"connect sdb " + nodeAddr, 0, e );
	}
	return dataDB;
}

function checkCappedCL ( dataDB, csName, clName )
{
	var expectName = csName + "." + clName;
	var isCappedCLExist = false;
	try
	{
		var cursor = dataDB.listCollections();
		while( cursor.next() )
		{
			var actName = cursor.current().toObj().Name;
			if( expectName == actName )
			{
				isCappedCLExist = true;
			}
		}

		if( !isCappedCLExist )
		{
			throw 'CHECK CAPPED CL FAIL';
		}
	}
	catch( e )
	{
		throw buildException( null, null, "checkCappedCL fail ", 0, e );
	}
}
main();