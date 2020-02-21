/************************************
*@Description：创建CL时指定domain和CS，分别用listXXX方法和getXXX方法检查对应的信息是否存在
*@Author：     Qiangzhong Deng  2015/10/14
**************************************/

var domainName = CHANGEDPREFIX + "_meta001_domain"
var csName = CHANGEDPREFIX + "_meta001_cs"
var clName = CHANGEDPREFIX + "_meta001_cl"

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
	metaOprCreateDomain( db, domainName, myDataGroups, false,
		"metaOpr.001: createDomain failed" );
	commCreateCS( db, csName, false, "", { Domain: domainName } );
	commCreateCL( db, csName, clName );
	//check test result
	checkTestResult( db, domainName, csName, clName );

	commDropCS( db, csName, false, "metaOpr.001: dropCS failed" );
	metaOprDropDomain( db, domainName, false, "metaOpr.001: dropDomain failed" );

	//check if there still exists domain
	metaOprCheckGetDomain( db, domainName, true,
		"metaOpr.001: getDomain(after dropDomain) succesfully." );
}
function checkInsert ( sdb, cs, cl )
{
	var clTmp = sdb.getCS( cs ).getCL( cl );
	try
	{
		clTmp.insert( { a: 1 } )
	}
	catch( e )
	{
		throw buildException( "metaOpr.001", e, "checkInsert", "insert successfully", "insert fail" );
	}

	try 
	{
		clTmp.find( { a: 1 } )
	}
	catch( e )
	{
		throw buildException( "metaOpr.001", e, "checkInsert", "find(after insert) successfully", "find(after insert) fail" );
	}
}
function checkTestResult ( sdb, dm, cs, cl )
{
	//check with listXXX methods
	metaOprCheckListDomains( sdb, dm );
	metaOprCheckListCLs( sdb, cs, cl );

	//check with getXXX methods
	metaOprCheckGetDomain( sdb, dm );
	metaOprCheckGetCL( sdb, cs, cl );

	checkInsert( sdb, cs, cl );
}