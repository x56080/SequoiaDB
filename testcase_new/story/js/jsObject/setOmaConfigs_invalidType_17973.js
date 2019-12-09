/******************************************************************************
*@Description : test Oma function: setOmaConfigs getIniConfigs                                       
*               TestLink: seqDB-17868:sdb和sdbcm的setOmaConfigs支持ini格式，ini字段数据类型测试 
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
		Oma.setOmaConfigs( { "inv.null": null }, filePath );
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
		Oma.setOmaConfigs( { "inv.null": [1, 2] }, filePath );
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
		oma.setOmaConfigs( { "inv.null": null }, filePath );
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
		oma.setOmaConfigs( { "inv.null": [1, 2] }, filePath );
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