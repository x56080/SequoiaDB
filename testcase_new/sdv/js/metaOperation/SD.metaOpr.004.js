/************************************
*@Description：更新域属性，分别用listXXX方法和getXXX方法检查对应的信息是否存在
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

	//get 4 data groups at most,the len is the actual groups length(maybe less than 4)
	var maxGroupsNumber = 4;
	var myDataGroups = metaOprGetDataGroups( db, maxGroupsNumber );
	var len = myDataGroups.length;
	if( len < 2 ) return;

	//begin test
	//pop one group and add it to domain at domain.alter later
	var addedGroupName = myDataGroups.pop();
	createDomainByOption( db, domainName, myDataGroups, { AutoSplit: true } );
	commCreateCS( db, csName, false, "", { Domain: domainName } );
	commCreateCL( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0 } );

	//check test result
	checkTestResult( db, domainName, csName, clName );

	//push the pop-out group first
	myDataGroups.push( addedGroupName );
	alterDomain( db, domainName, { Groups: myDataGroups } );

	//check that the new insertation records doesnot fall into the newly-added group
	checkDataLocation( db, csName, clName, addedGroupName );

	//clear test environment
	commDropCS( db, csName, false, "metaOpr.004: dropCS failed" );
	metaOprDropDomain( db, domainName, false, "metaOpr.004: dropDomain failed" );
}
/* *****************************************************************************
@discription: check if data DOESNOT exist on the new datagroup added by domain.alter
@parameter
	sdb:database connection
	cs:collection spaces
   cl:collection
   dg:newly added data group
***************************************************************************** */
function checkDataLocation ( sdb, cs, cl, dg )
{
	var clTmp = sdb.getCS( cs ).getCL( cl );
	try
	{
		for( var i = 0; i < 100; i++ )
		{
			clTmp.insert( { a: i } );
		}
	}
	catch( e )
	{
		throw buildException( "metaOpr.004", e, "checkDataLocation", "insert successfully", "insert fail" );
	}
	var tmpInfo;
	try
	{
		tmpInfo = sdb.list( SDB_LIST_GROUPS ).toArray();
	}
	catch( e )
	{
		throw buildException( "metaOpr.004", e, "checkDataLocation", "list groups successfully", "list groups fail" );
	}

	for( var j = 0; j < tmpInfo.length; j++ )
	{
		var tmpObj = eval( "(" + tmpInfo[j] + ")" );
		if( tmpObj.GroupName !== dg )
		{
			continue;
		}
		var hName = tmpObj.Group[0].HostName;
		var svcName = tmpObj.Group[0].Service[0].Name;
		var tmpDB = new Sdb( hName, svcName );
		metaOprCheckGetCS( tmpDB, cs, true,
			"db(" + hName + "," + svcName + ").getCS() success, which should fail anyway" );
		tmpDB.close();
	}
}
function createDomainByOption ( sdb, dm, dg, option )
{
	try
	{
		sdb.createDomain( dm, dg, option );
	}
	catch( e )
	{
		throw buildException( "metaOpr.004", e, "createDomainByOption", "createDomain successfully", "createDomain fail" );
	}
}
function checkTestResult ( sdb, dm, cs, cl )
{
	//check using listXXX methods
	metaOprCheckListDomains( sdb, dm );
	metaOprCheckListCLs( sdb, cs, cl );

	//check using getXXX methods	
	metaOprCheckGetDomain( sdb, dm );
	metaOprCheckGetCL( sdb, cs, cl );
}
function alterDomain ( sdb, dm, groupObj )
{
	var myDomain = sdb.getDomain( dm );
	try
	{
		myDomain.alter( groupObj );
	}
	catch( e )
	{
		throw buildException( "metaOpr.004", e, "alterDomain", "domain.alter sucessfully", "domain.alter fail" );
	}
}