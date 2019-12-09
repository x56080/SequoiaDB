/**********************************************
@Description: repeat stop-start data node
@author:      LY
**********************************************/

function createDataNode ( db, hostname, port, group )
{
	if( undefined === db ) 
	{
		throw ( "the db is null, error" );
	}
	if( undefined === hostname ) 
	{
		throw ( "the hostname is null, error" );
	}
	if( undefined === port ) 
	{
		throw ( "the port is null, error" );
	}
	if( undefined === group ) 
	{
		throw ( "the group is null, error" );
	}

	try
	{
		var dataRG = db.createRG( group );
		dataRG.createNode( hostname, port, SPAREPORTPATH + port );
	}
	catch( e )
	{
		throw buildException(
			"build data node",
			e,
			"var dataRG = db.createRG( " + group + " ); dataRG.createNode( " + hostname
			+ ", " + port + ", " + SPAREPORTPATH + port + " )",
			"succ",
			e
		);
	}
}

function stopDataNode ( db, hostname, port, group )
{
	if( undefined === db ) 
	{
		throw ( "the db is null, error" );
	}
	if( undefined === hostname ) 
	{
		throw ( "the hostname is null, error" );
	}
	if( undefined === port ) 
	{
		throw ( "the port is null, error" );
	}
	if( undefined === group ) 
	{
		throw ( "the group is null, error" );
	}

	try
	{
		var dataRG = db.getRG( group );
		var dataNode = dataRG.getNode( hostname, port );
		dataNode.stop();
	}
	catch( e )
	{
		throw buildException(
			"build data node",
			e,
			"var dataRG = db.getRG( " + group + " ); var dataNode = dataRG.getNode( "
			+ hostname + ", " + port + " ); dataNode.stop() ;",
			"succ",
			"fail"
		);
	}
}

function startDataNode ( db, hostname, port, group )
{
	if( undefined === db ) 
	{
		throw ( "the db is null, error" );
	}
	if( undefined === hostname ) 
	{
		throw ( "the hostname is null, error" );
	}
	if( undefined === port ) 
	{
		throw ( "the port is null, error" );
	}
	if( undefined === group ) 
	{
		throw ( "the group is null, error" );
	}

	try
	{
		var dataRG = db.getRG( group );
		var dataNode = dataRG.getNode( hostname, port );
		dataNode.start();
	}
	catch( e )
	{
		throw buildException(
			"build data node",
			e,
			"var dataRG = db.getRG( " + group + " ); var dataNode = dataRG.getNode( "
			+ hostname + ", " + port + " ); dataNode.start() ;",
			"succ",
			"fail"
		);
	}
}

function checkDataNode ( db, group )
{
	// clean collection in the beginning
	commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "clear collection in the beginning" );
	try
	{
		//build collection
		var cl = commCreateCLByOption( db, COMMCSNAME, COMMCLNAME, { Group: group }, false, true, "create collection in the beginning" );
		// insert data and check result
		insertAndCheck( cl, 10000, "check collection records in the end, error" );
		// clean collection after test correct
		commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "clear collection in the end, correct" );
	}
	catch( e )
	{
		throw buildException(
			"build collection and insert records",
			e,
			"collection name : " + cl,
			"succ",
			"failure"
		);
	}
}

function main ( db )
{
	try
	{
		if( commIsStandalone( db ) )
		{
			println( "run mode is standalone" );
			return;
		}
		// create data node port
		var data = generatePort( SPAREPORTSTART );
		// create dataGroup name
		var groupName = CHANGEDPREFIX + "_group";
		// get new cluster used hostname 
		var hostname = getHostName();
		// set the number of cycles
		createDataNode( db, hostname, data, groupName );
		var num = 5;
		for( var i = 0; i < num; ++i )
		{
			// stop data node in the original data group
			stopDataNode( db, hostname, data, groupName );
			// start data node in the creating data group
			startDataNode( db, hostname, data, groupName );
		}
		// check data node
		checkDataNode( db, groupName );
		// clean 
		clean( db, hostname, data, groupName, "data_group" );
	}
	catch( e )
	{
		throw e;
	}
	finally
	{
		if( undefined !== db )
		{
			db.close();
		}
	}
}

//run main
main( db );