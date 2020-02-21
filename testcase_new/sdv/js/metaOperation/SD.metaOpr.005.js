/************************************
*@Description：修改集合属性，并检查修改是否成功
*@Author：     Qiangzhong Deng  2015/10/14
@modify:       zhaoyu 2016.5.18,
               modify:get data from master node 
**************************************/

var domainName = CHANGEDPREFIX + "_domain"
var csName = CHANGEDPREFIX + "_meta005_cs"
var clName = CHANGEDPREFIX + "_meta005_cl"

main( db );
function main ( db )
{
	if( commIsStandalone( db ) ) return;

	//prepare test environment
	commDropCS( db, csName, true );
	metaOprDropDomain( db, domainName, true, "Delete domain before test" );

	//get 2 data groups at most,the len is the actual groups length(maybe less than 2)
	var maxGroupsNumber = 2;
	var myDataGroups = metaOprGetDataGroups( db, maxGroupsNumber );
	var len = myDataGroups.length;
	if( len != maxGroupsNumber ) return;

	//begin test
	metaOprCreateDomain( db, domainName, myDataGroups );
	commCreateCS( db, csName, false, "", { Domain: domainName } );
	//commCreateCL( db, csName, clName );
	commCreateCL( db, csName, clName, { ReplSize: 0 } );

	//alter cl and split it
	var splitGroups = alterCL( db, csName, clName, myDataGroups );

	//check test result:connect to data nodes to check that the data falls into right data node.
	println( "--Begin to alter CL" );
	checkDataLocation( db, csName, clName, splitGroups );
	println( "--end alter CL" );
	//clear test environment
	commDropCS( db, csName, false, "metaOpr.005: dropCS failed" );
	metaOprDropDomain( db, domainName, false, "metaOpr.005: dropDomain failed" );
}
/* *****************************************************************************
@discription: bulk insert
@author: Qiangzhong Deng
@parameter
	num:the number of insertation
***************************************************************************** */
function bulkInsert ( sdb, cs, cl, num )
{
	var tmpCL = sdb.getCS( cs ).getCL( cl );
	try
	{
		for( var i = 0; i < num; i++ )
		{
			tmpCL.insert( { a: i + 1, b: "sequoiadb" } );
		}
	}
	catch( e )
	{
		throw buildException( "metaOpr.005", e, "bulkInsert", "bulkInsert success.", "bulkInsert fail" );
	}
}
/* *****************************************************************************
@discription: 1.insert 1000 records to cl
              2.alter cl attributes
              3.split cl
@author: Qiangzhong Deng
@parameter:
@return:splitGroups,a array that include the two data groups used in split,
        splitGroups[0] is the source group
        splitGroups[1] is the target group
***************************************************************************** */
function alterCL ( sdb, cs, cl, dg )
{
	var insertNum = 1000;
	bulkInsert( sdb, cs, cl, insertNum );
	var tmpCL = sdb.getCS( cs ).getCL( cl );
	try
	{
		println( "--Begin to alter CL" );
		tmpCL.alter( { ShardingKey: { a: 1 }, ShardingType: "range" } );
		println( "--end alter CL" );
	}
	catch( e )
	{
		throw buildException( "metaOpr.005", e, "alterCL", "alterCL success", "alterCL fail" );
	}
	var srcGroups = commGetCLGroups( sdb, cs + "." + cl );
	var dstGroup;
	//dg.length must be 2
	if( srcGroups[0] === dg[0] )
	{
		dstGroup = dg[1];
	}
	else
	{
		dstGroup = dg[0];
	}
	//percentage split
	try
	{
		tmpCL.split( srcGroups[0], dstGroup, 50 );
	}
	catch( e )
	{
		throw buildException( "metaOpr.005", e, "alterCL", "split success", "split fail" );
	}
	var splitGroups = new Array();
	splitGroups.push( srcGroups[0] );
	splitGroups.push( dstGroup );
	return splitGroups;
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
function checkDataLocation ( sdb, cs, cl, splitGroups )
{
	for( var i = 0; i < splitGroups.length; i++ )
	{
		//get data from master node;  
		var getMasterNode = sdb.getRG( splitGroups[i] ).getMaster().toString().split( ":" );
		var tmpDB = new Sdb( getMasterNode[0], getMasterNode[1] );
		var tmpCL = tmpDB.getCS( cs ).getCL( cl );
		var cnt = tmpCL.count( { a: { $gt: 500 * i }, a: { $lte: 500 + 500 * i } } );
		try
		{
			if( Number( cnt ) !== 500 )
			{
				throw buildException( "metaOpr.005", -1, "count()", "returns 500", "doesnot return 500" );
			}
		}
		catch( e )
		{
			throw e;
		}
	}
}