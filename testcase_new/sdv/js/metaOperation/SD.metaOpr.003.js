/************************************
*@Description：创建CL指定domain，且domain指定自动切分，分别用listXXX方法和
               getXXX方法检查对应的信息是否存在
*@Author：     Qiangzhong Deng  2015/10/14
**************************************/

var domainName = CHANGEDPREFIX + "_domain"
var csName = CHANGEDPREFIX + "_cs"
var clName = CHANGEDPREFIX + "_cl"

main( db );
function main ( db )
{
	if( commIsStandalone( db ) ) return;

	//prepare test environment
	commDropCS( db, csName, true );
	metaOprDropDomain( db, domainName, true, "Delete domain before test" );

	//get 3 data groups at most,the len is the actual groups length(maybe less than 3)
	var maxGroupsNumber = 3;
	var myDataGroups = metaOprGetDataGroups( db, maxGroupsNumber );
	var len = myDataGroups.length;
	if( len < 1 ) return;

	//begin test
	createDomainByOption( db, domainName, myDataGroups, { AutoSplit: true } );
	commCreateCS( db, csName, false, "", { Domain: domainName } );
	commCreateCLByOption( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0 } );

	//check test result 
	checkTestResult( db, domainName, csName, clName, myDataGroups );

	//clear test environment
	commDropCS( db, csName, false, "metaOpr.003: dropCS failed" );
	metaOprDropDomain( db, domainName, false, "metaOpr.003: dropDomain failed" );
}
function checkCLWithSnapshot ( sdb, cs, cl )
{
	var tmpInfo;
	try 
	{
		tmpInfo = sdb.snapshot( SDB_SNAP_CATALOG ).toArray();
	}
	catch( e )
	{
		throw buildException( "metaOpr.003", e, "checkCLWithSnapshot",
			"snapshot(SDB_SNAP_CATALOG) sucessfully", "snapshot(SDB_SNAP_CATALOG) fail" );
	}

	var flag = 0;
	for( var i = 0; i < tmpInfo.length; i++ )
	{
		var tmpObj = eval( "(" + tmpInfo[i] + ")" );
		if( tmpObj.Name === csName + "." + clName &&
			tmpObj.Partition === 1024 &&
			tmpObj.ShardingType === "hash" )
		{
			flag = 1;
			break;
		}
	}
	try
	{
		if( flag == 0 )
		{
			throw buildException( "metaOpr.003", -1, "checkCLWithSnapshot",
				"catalog info of " + csName + "." + clName + " exist",
				"catalog info of " + csName + "." + clName + " doesn't exist" );
		}
	}
	catch( e )
	{
		throw e;
	}
}
/* *****************************************************************************
@discription: check if data exist on corresponding datagroups in domain
@parameter
	sdb:database connection
	cs:collection spaces
   cl:collection
   dg:data groups array of all the data groups in the cluster
   l :length of dg to be checked
***************************************************************************** */
function checkDataLocation ( sdb, cs, cl, dg )
{
	var sleepInteval = 10;
	var sleepDuration = 0;
	var maxSleepDuration = 100;
	var insertNum = 100;
	var clTmp = sdb.getCS( cs ).getCL( cl );
	try
	{
		for( var i = 0; i < insertNum; i++ )
		{
			clTmp.insert( { a: i } )
		}
	}
	catch( e )
	{
		throw buildException( "metaOpr.003", e, "checkDataLocation", "insert successfully", "insert fail" );
	}
	//each count() of data groups doesnot equal to 0 and
	//sum of these count() equal to insertation number( i.e. 100 )
	var total = 0;
	var tmpInfo;
	var l = dg.length;
	try
	{
		tmpInfo = sdb.list( SDB_LIST_GROUPS ).toArray();
	}
	catch( e )
	{
		throw buildException( "metaOpr.003", e, "checkDataLocation", "list groups successfully", "list groups fail" );
	}
	for( var i = 0; i < l; i++ )
	{
		var dgroup = dg[i];
		for( var j = 0; j < tmpInfo.length; j++ )
		{
			var tmpObj = eval( "(" + tmpInfo[j] + ")" );
			if( tmpObj.GroupName !== dgroup )
			{
				continue;
			}
			var hName = tmpObj.Group[0].HostName;
			var svcName = tmpObj.Group[0].Service[0].Name;
			var tmpDB = new Sdb( hName, svcName );
			var cnt = eval( "tmpDB." + cs + "." + cl + ".count()" );
			while( parseInt( cnt ) === 0 && sleepDuration < maxSleepDuration )
			{
				sleep( sleepInteval );
				sleepDuration += sleepInteval;
			}
			try
			{
				if( parseInt( cnt ) === 0 )
				{
					println( "tmpDB(" + hName + "," + svcName + ")." + cs + "." + cl + ".count() returns 0" );
					throw buildException( "metaOpr.003", -1, "checkDataLocation",
						"count returns non-zero", "count returns zero" );
				}
				total += cnt;
			}
			catch( e )
			{
				throw e;
			}
			finally 
			{
				tmpDB.close();
			}
		}
	}
	try
	{
		if( total !== insertNum )
		{
			println( "total: " + total + " insertNum:" + insertNum );
			throw buildException( "metaOpr.003", -1, "checkDataLocation",
				"total number equals 100", "total number doesnot equal 100" );
		}
	}
	catch( e )
	{
		throw e;
	}
}
function checkTestResult ( sdb, dm, cs, cl, dg )
{
	//check using listXXX methods
	metaOprCheckListDomains( sdb, dm );
	metaOprCheckListCLs( sdb, cs, cl );

	//check using getXXX methods
	metaOprCheckGetDomain( sdb, dm );
	metaOprCheckGetCL( sdb, cs, cl );

	checkCLWithSnapshot( sdb, cs, cl );
	checkDataLocation( sdb, cs, cl, dg );
}
function createDomainByOption ( sdb, dm, dg, option )
{
	try
	{
		sdb.createDomain( dm, dg, option );
	}
	catch( e )
	{
		throw buildException( "metaOpr.003", e, "createDomainByOption", "createDomain successfully", "createDomain fail" );
	}
}