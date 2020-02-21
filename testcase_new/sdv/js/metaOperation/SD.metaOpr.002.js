/************************************
*@Description：创建CL不指定domain，分别用listXXX方法和getXXX方法检查对应的信息是否存在
*@Author：     Qiangzhong Deng  2015/10/14
**************************************/

var csName = CHANGEDPREFIX + "_meta002_cs"
var clName = CHANGEDPREFIX + "_meta002_cl"

main( db );
function main ( db )
{
	if( commIsStandalone( db ) ) return;
	//prepare test environment
	commDropCS( db, csName, true );

	//begin test
	commCreateCS( db, csName );
	commCreateCL( db, csName, clName );

	//check test result
	checkTestResult( db, csName, clName );

	//clear test environment
	commDropCL( db, csName, clName );
	commDropCS( db, csName, false, "metaOpr.002: dropCS failed" );
}

function checkCLOnCatalog ( cdb, cs, cl, message )
{
	if( message == undefined ) { message = ""; }
	try
	{
		var data = cdb.SYSCAT.SYSCOLLECTIONS.find().toArray();
		var flag = 0;
		for( var i = 0; i < data.length; i++ )
		{
			var tmpObj = eval( "(" + data[i] + ")" );
			if( tmpObj["Name"] === cs + "." + cl )
			{
				flag = 1;
				break;
			}
		}
		if( flag === 0 )
		{
			throw buildException( "metaOpr.002", -1, "checkCLOnCatalog",
				"catalog info of " + cs + "." + cl + " exist.",
				"catalog info of " + cs + "." + cl + " doesn't exist" );
		}
	}
	catch( e )
	{
		throw e;
	}
}
function checkTestResult ( sdb, cs, cl )
{
	//check using listXXX methods
	metaOprCheckListCLs( sdb, cs, cl );

	//check using getXXX methods
	metaOprCheckGetCL( sdb, cs, cl );

	//check on the catalog primary node
	var host = COORDHOSTNAME;
	var svc = CATASVCNAME;
	var replicaGroups = commGetGroups( sdb, false, "", false, true, true );
	for( var i = 0; i < replicaGroups.length; i++ )
	{
		var tmpObj = replicaGroups[i];
		var groupName = tmpObj[0].GroupName;
		if( groupName === "SYSCatalogGroup" )
		{
			var primaryPos = tmpObj[0].PrimaryPos;
			host = tmpObj[primaryPos].HostName;
			svc = tmpObj[primaryPos].svcname;
			println( "metaOpr.002 checkTestResult get SYSCatalogGroup primary node successfully" );
		}
	}
	var catadb = new Sdb( host, svc );
	checkCLOnCatalog( catadb, cs, cl );
	catadb.close();
}
