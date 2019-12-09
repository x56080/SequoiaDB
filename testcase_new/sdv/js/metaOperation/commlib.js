/* *****************************************************************************
@discription: get maxNum(at most) data groups
@author: Qiangzhong Deng
@parameter
	maxNum:the max number of data groups returned
***************************************************************************** */
function metaOprGetDataGroups ( db, maxNum )
{
	if( commIsStandalone( db ) )
	{
		return new Array();
	}
	var myArray = new Array();
	var tmpInfo;
	try
	{
		tmpInfo = db.list( SDB_LIST_GROUPS ).toArray();
	}
	catch( e )
	{
		throw buildException( "metaOpr commlib.js", e, "metaOprGetDataGroups",
			"list(SDB_LIST_GROUPS) sucess", "list(SDB_LIST_GROUPS) fail" );
	}
	for( var i = 0; i < tmpInfo.length; i++ )
	{
		var tmpObj = eval( "(" + tmpInfo[i] + ")" );
		if( tmpObj.Role !== 0 )
		{
			continue;
		}
		myArray.push( tmpObj.GroupName );
		if( myArray.length >= maxNum )
		{
			break;
		}
	}
	return myArray;
}

/* *****************************************************************************
@discription: delete domain
@author: Qiangzhong Deng
@parameter
	dmName:the domain to be deleted
   ignoreExisted: default = true, value: true/false
   message: user define message, default:""
***************************************************************************** */
function metaOprDropDomain ( db, dmName, ignoreNotExist, message )
{
	if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
	if( message == undefined ) { message = ""; }
	try
	{
		db.dropDomain( dmName );
	}
	catch( e )
	{
		if( e === -214 && ignoreNotExist )
		{
			// think right
		}
		else
		{
			throw buildException( "metaOpr commlib.js", e, message, "dropDomain sucessfully", "dropDomain fail" );
		}
	}
}
/* *****************************************************************************
@discription: check if domain exists using listXXX methods
@author: Qiangzhong Deng
@parameter
	expectDomain:the domain to be checked
***************************************************************************** */
function metaOprCheckListDomains ( db, expectDomain )
{
	var tmpInfo;
	try
	{
		tmpInfo = db.listDomains().toArray();
	}
	catch( e )
	{
		throw buildException( "metaOpr commlib.js", e, "metaOprCheckListDomains", "listDomains() sucessfully", "listDomains() fail" );
	}
	var flag = 0;
	for( var i = 0; i < tmpInfo.length; i++ )
	{
		var tmpObj = eval( "(" + tmpInfo[i] + ")" );
		if( tmpObj.Name === expectDomain )
		{
			flag = 1;
			break;
		}
	}
	try
	{
		if( flag === 0 )
		{
			throw buildException( "metaOpr commlib.js", -1, "metaCheckListDomains",
				"expectDomain " + expectDomain + " exist",
				"expectDomain " + expectDomain + " doesn't exist" );
		}
	}
	catch( e )
	{
		throw e;
	}
}
/* *****************************************************************************
@discription: check if cs.cl exists using listXXX methods
@author: Qiangzhong Deng
@parameter
	expectCS:the cs to be checked
	expectCL:the cl to be checked
***************************************************************************** */
function metaOprCheckListCLs ( db, expectCS, expectCL )
{
	var tmpInfo = db.listCollections().toArray();
	var flag = 0;
	for( var i = 0; i < tmpInfo.length; i++ )
	{
		var tmpObj = eval( "(" + tmpInfo[i] + ")" );
		if( tmpObj.Name === expectCS + "." + expectCL )
		{
			flag = 1;
			break;
		}
	}
	try
	{
		if( flag === 0 )
		{
			throw buildException( "metaOpr commlib.js", -1, "metaCheckListCLs",
				"expectCL " + expectCS + "." + expectCL + " exist",
				"expectCL " + expectCS + "." + expectCL + " doesn't exist" );
		}
	}
	catch( e )
	{
		throw e;
	}
}
/* *****************************************************************************
@discription: create domain
@author: Qiangzhong Deng
@parameter
	dmName:the domain to be deleted
	dataGroups:data groups used, format:['datagroup1','datagroup2']
   ignoreExisted: default = false, value: true/false
   message: user define message, default:""
***************************************************************************** */
function metaOprCreateDomain ( db, dmName, dataGroups, ignoreExist, message )
{
	if( ignoreExist == undefined ) { ignoreExist = false; }
	if( message == undefined ) { message = ""; }
	try
	{
		db.createDomain( domainName, dataGroups );
	}
	catch( e )
	{
		if( e === -215 && ignoreExist )
		{
			//right situation, so do nothing
		}
		else
		{
			throw buildException( "metaOpr commlib.js", e, "metaOprCreateDomain", "createDomain successfully", "createDomain fail" );
		}
	}
}
/* *****************************************************************************
@discription: check if domain exists using getDomain methods
@author: Qiangzhong Deng
@parameter
	expectDomain:the domain to be checked
	ignoreNotExist:default = false, value: true/false
	message: user define message, default:""
***************************************************************************** */
function metaOprCheckGetDomain ( db, expectDomain, ignoreNotExist, message )
{
	if( ignoreNotExist == undefined ) { ignoreNotExist = false; }
	if( message == undefined ) { message = ""; }
	try
	{
		db.getDomain( expectDomain );
	}
	catch( e )
	{
		if( e === -214 && ignoreNotExist )
		{
			//right situation, so do nothing
		}
		else
		{
			throw buildException( "metaOpr commlib.js", e, "metaOprCheckGetDomain", "getDomain successfully", "getDomain fail" );
		}
	}
}
/* *****************************************************************************
@discription: check if cs exists using getCS method
@author: Qiangzhong Deng
@parameter
	expectCS:cs to be checked
	ignoreNotExist:default = false, value: true/false
	message: user define message, default:""
***************************************************************************** */
function metaOprCheckGetCS ( db, expectCS, ignoreNotExist, message )
{
	if( ignoreNotExist == undefined ) { ignoreNotExist = false; }
	if( message == undefined ) { message = ""; }
	try
	{
		db.getCS( expectCS );
	}
	catch( e )
	{
		if( e === -34 && ignoreNotExist )
		{
			//right situation, so do nothing
		}
		else
		{
			throw buildException( "metaOpr commlib.js", e, "metaOprCheckGetCS", "getCS successfully", "getCS fail" );
		}
	}
}
/* *****************************************************************************
@discription: check if cl exists using getCL method
@author: Qiangzhong Deng
@parameter
	expectCS:cs to be checked
	expectCL:cl to be checked
	ignoreNotExist:default = false, value: true/false
	message: user define message, default:""
***************************************************************************** */
function metaOprCheckGetCL ( db, expectCS, expectCL, ignoreNotExist, message )
{
	if( ignoreNotExist == undefined ) { ignoreNotExist = false; }
	if( message == undefined ) { message = ""; }
	var tmpCS;
	try
	{
		tmpCS = db.getCS( expectCS );
	}
	catch( e )
	{
		throw buildException( "metaOpr commlib.js", e, "metaOprCheckGetCL", "getCS successfully", "getCS fail" );
	}
	try
	{
		tmpCS.getCL( expectCL );
	}
	catch( e )
	{
		if( e === -23 && ignoreNotExist )
		{
			//right situation, so do nothing
		}
		else
		{
			throw buildException( "metaOpr commlib.js", e, "metaOprCheckGetCL", "getCL successfully", "getCL fail" );
		}
	}
}