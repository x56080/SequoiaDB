/************************************
*@Description：在空域创建CS，检查是否出错
*@Author：     Qiangzhong Deng  2015/10/14
**************************************/

var domainName = CHANGEDPREFIX + "_meta006_domain"
var csName = CHANGEDPREFIX + "_meta006_cs"

main( db );
function main ( db )
{
	if( commIsStandalone( db ) ) return;
	commDropCS( db, csName, true );
	metaOprDropDomain( db, domainName, true, "Delete domain before test" );

	//create a domain that doesnot include any data groups
	metaOprCreateDomain( db, domainName, [], false, "metaOpr.006: createDomain failed" );
	try
	{
		db.createCS( csName, { Domain: domainName } );
	}
	catch( e )
	{
		if( e !== -262 )
		{
			println( "createCS in empty domain success, which should fail" );
			throw e;
		}
	}
	metaOprDropDomain( db, domainName, false, "metaOpr.006: dropDomain failed" );
}
