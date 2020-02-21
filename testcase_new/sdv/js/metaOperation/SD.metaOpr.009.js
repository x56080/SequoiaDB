/************************************
*@Description：domain不为空，执行drop操作
*@Author：     Qiangzhong Deng  2015/10/14
**************************************/

var domainName = CHANGEDPREFIX + "_meta009_domain"
var csName = CHANGEDPREFIX + "_meta009_cs"

main( db );
function main ( db )
{
	if( commIsStandalone( db ) ) return;

	commDropCS( db, csName, true );
	metaOprDropDomain( db, domainName, true, "Delete domain before test" );

	//get 3 data groups at most,the len is the actual groups length(maybe less than 3)
	var maxGroupsNumber = 3;
	var myDataGroups = metaOprGetDataGroups( db, maxGroupsNumber );
	var len = myDataGroups.length;
	if( len < 1 ) return;

	//create a domain that include a collection space
	metaOprCreateDomain( db, domainName, myDataGroups, false, "metaOpr.009: createDomain failed" );
	commCreateCS( db, csName, false, "", { Domain: domainName } );

	//try to drop domain that is not empty
	try
	{
		db.dropDomain( domainName );
	}
	catch( e )
	{
		if( e !== -256 )
		{
			println( "Drop non-empty domain successfully, which should fail anyway" );
			throw e;
		}
	}

	//clear test environment
	commDropCS( db, csName, false, "metaOpr.009: dropCS failed" );
	metaOprDropDomain( db, domainName, false, "metaOpr.009: dropDomain failed" );
}