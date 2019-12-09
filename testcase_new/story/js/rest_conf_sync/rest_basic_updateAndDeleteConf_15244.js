/******************************************************************************
 * @Description : test update and delete run config 
 *                seqDB-15244:更新删除节点配置
 * @author      : Fan YU
 * @date        ：2018.10.17
 ******************************************************************************/
main();
function main ()
{
	if( commIsStandalone( db ) )
	{
		println( "run mode is standalone" );
		return;
	}
	//get a group in cluster
	var groupName = getDataGroups( db )[0];
	//get a node of the group in cluster
	var nodePort = getGroupNodes( db, groupName )[0];
	//configure need to update and delete
	var config = "{auditnum:25}";
	//the options of snapshot and configure
	var options = "{GroupName:\"" + groupName + "\",SvcName:\"" + nodePort + "\"}";
	var selector = "{auditnum:" + null + "}";
	//delete configure
	deleteConf( config, options );
	//get the default value of auditnum
	var defval = getCurrentConfBySnapShot( options, selector );

	//update configure
	updateConf( config, options );
	//get current configure value after update configure
	var actval = getCurrentConfBySnapShot( options, selector );
	//check result
	checkResult( config, actval );

	//delete configure
	deleteConf( config, options );
	//get current configure value after delete configure
	var actval1 = getCurrentConfBySnapShot( options, selector );
	//check result
	checkResult( defval, actval1 );
}

function updateConf ( configs, options )
{
	println( "\n---Begin to excute " + getFuncName() );
	var curlPara = ['cmd=update config',
		'configs=' + configs,
		'options=' + options];
	var expErrno = 0;
	runCurl( curlPara );
	var resp = infoSplit;
	var actErrno = JSON.parse( resp[0] ).errno;
	if( expErrno != actErrno )
	{
		throw "update Configure failed,info = " + infoSplit.toString();
	}
}

function deleteConf ( configs, options )
{
	println( "\n---Begin to excute " + getFuncName() );
	var curlPara = ['cmd=delete config',
		'configs=' + configs,
		'options=' + options];
	var expErrno = 0;
	runCurl( curlPara );
	var resp = infoSplit;
	var actErrno = JSON.parse( resp[0] ).errno;
	if( expErrno != actErrno )
	{
		throw "delete Configure failed,info = " + infoSplit.toString();
	}
}

function getCurrentConfBySnapShot ( options, selector )
{
	println( "\n---Begin to excute " + getFuncName() );
	var curlPara = ['cmd=snapshot configs',
		"filter=" + options,
		"selector=" + selector];
	var expErrno = 0;
	runCurl( curlPara );
	var resp = infoSplit;
	var actErrno = JSON.parse( resp[0] ).errno;
	if( expErrno != actErrno )
	{
		throw "get configures by snapshot failed,info = " + infoSplit.toString();
	}
	resp.shift();
	return resp;
}

function checkResult ( expInfo, actInfo )
{
	println( "\n---Begin to excute " + getFuncName() );
	var expVal = JSON.parse( expInfo ).auditnum;
	var actVal = JSON.parse( actInfo ).auditnum;
	if( expVal != actVal )
	{
		throw "expVal is not equal to actVal,expInfo = " + expInfo + ",actInfo = " + actInfo;
	}
}

function getDataGroups ( db )
{
	println( "\n---Begin to excute " + getFuncName() );
	var groups = [];
	if( commIsStandalone( db ) )
	{
		return groups;
	}
	var cursor = db.listReplicaGroups();
	var tmpInfo;
	while( tmpInfo = cursor.next() )
	{
		var groupName = tmpInfo.toObj().GroupName;
		if( groupName == "SYSCoord" || groupName == "SYSCatalogGroup" )
			continue;
		groups.push( groupName );
	}
	return groups;
}

function getGroupNodes ( db, rgName )
{
	println( "\n---Begin to excute " + getFuncName() );
	var nodes = [];
	if( commIsStandalone( db ) )
	{
		return nodes;
	}
	var tmpObj = db.getRG( rgName ).getDetail().next().toObj();
	var tmpGroupArray = tmpObj["Group"];
	for( var i = 0; i < tmpGroupArray.length; i++ )
	{
		var tmpNodeObj = tmpGroupArray[i];
		var nodename = tmpNodeObj["HostName"];
		for( var j = 0; j < tmpNodeObj.Service.length; j++ )
		{
			var tmpSvcObj = tmpNodeObj.Service[j];
			if( tmpSvcObj["Type"] == 0 )
			{
				nodename = tmpSvcObj["Name"];
				nodes.push( nodename );
				break;
			}
		}
	}
	return nodes;
}

