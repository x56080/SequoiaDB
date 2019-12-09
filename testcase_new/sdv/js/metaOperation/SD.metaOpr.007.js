/************************************
*@Description：创建CL且指定组不在所属域内
*@Author：     Qiangzhong Deng  2015/10/14
**************************************/

var domainName = CHANGEDPREFIX + "_domain"
var csName = CHANGEDPREFIX + "_cs"
var clName = CHANGEDPREFIX + "_cl"

main( db );
function main ( db )
{
	if( commIsStandalone( db ) ) return;
	commDropCS( db, csName, true );
	metaOprDropDomain( db, domainName, true, "Delete domain before test" );

	//get 2 data groups at most,the len is the actual groups length(maybe less than 2)
	var maxGroupsNumber = 2;
	var myDataGroups = metaOprGetDataGroups( db, maxGroupsNumber );
	var len = myDataGroups.length;
	if( len != maxGroupsNumber ) return;

	//create a domain that includes one data group
	metaOprCreateDomain( db, domainName, new Array( myDataGroups[0] ), false,
		"metaOpr.007: createDomain failed" );
	var tmpCS = commCreateCS( db, csName, false, "", { Domain: domainName } );
	try 
	{
		tmpCS.createCL( clName, { Group: myDataGroups[1] } );
	}
	catch( e )
	{
		if( e !== -216 )
		{
			println( "metaOpr.007 createCL success, which is expected to failed" );
			throw e;
		}
	}
	commDropCS( db, csName, false, "metaOpr.007: dropCS failed" );
	metaOprDropDomain( db, domainName, false, "metaOpr.007: dropDomain failed" );
}