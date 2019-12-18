/* *****************************************************************************
@Description: Deployment standalone mode
@author:      LY
***************************************************************************** */

function main ( db )
{
	// generate standalone data port
	var standaloneDataPort = generatePort( SPAREPORTSTART, "" );
	// build standalone data and connect it
	var tmpDb = connectSdb( COORDHOSTNAME, CMSVCNAME, standaloneDataPort, true );
	// clear collection in the beginning
	commDropCL( tmpDb, COMMCSNAME, COMMCLNAME, true, true, "clear collection in the beginning" );
	try
	{
		//build collection
		var cl = commCreateCL( tmpDb, COMMCSNAME, COMMCLNAME, {}, true, false, "create collection in the beginning" );
		// insert data and check result
		insertAndCheck( cl, 10000, "check collection records in the end, error" );
		// clear collection in the end when its correct
		commDropCL( tmpDb, COMMCSNAME, COMMCLNAME, false, false, "clear collection in the end, correct" );
		// clear standalone data after test correct
		clean( tmpDb, COORDHOSTNAME, standaloneDataPort, "", "standalone_data" );
	}
	catch( e )
	{
		throw buildException( "build collection and insert records", e, "collection name : " + cl, "succ", "failure" );
	}
	finally
	{
		db.close();
	}
}

// run main
main( db );