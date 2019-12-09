/******************************************************************************
*@Description : test Oma function: setIniConfigs getIniConfigs                                       
*               TestLink: seqDB-17868:sdb和sdbcm的setIniConfigs支持ini格式，ini字段数据类型测试 
*@Author      : 2019-3-7  XiaoNi Huang
*@Info		  : invalid type[null/array/......]
******************************************************************************/
main();

function main ()
{
	println( "\n---Begin to run test" );
	var filePath = WORKDIR + "/" + "config17973_sdbcm.conf";

	// sdb test
	// invalid type[null]
	try
	{
		Oma.setIniConfigs( { "inv.null": null }, filePath );
		throw "expect fail but actual succ.";
	}
	catch( e )
	{
		if( -6 != e ) 
		{
			throw e;
		}
	}

	// invalid type[array]
	try
	{
		Oma.setIniConfigs( { "inv.null": [1, 2] }, filePath );
		throw "expect fail but actual succ.";
	}
	catch( e )
	{
		if( -6 != e ) 
		{
			throw e;
		}
	}


	// sdbcm test
	var oma = new Oma( COORDHOSTNAME, CMSVCNAME );

	// invalid type[null]
	try
	{
		oma.setIniConfigs( { "inv.null": null }, filePath );
		throw "expect fail but actual succ.";
	}
	catch( e )
	{
		if( -6 != e ) 
		{
			throw e;
		}
	}

	// invalid type[array]
	try
	{
		oma.setIniConfigs( { "inv.null": [1, 2] }, filePath );
		throw "expect fail but actual succ.";
	}
	catch( e )
	{
		if( -6 != e ) 
		{
			throw e;
		}
	}

	oma.close();

}