/****************************************************
@description:	insert, normal case
         testlink cases: seqDB-7198
@input:		run CURL command: 
				curl http://127.0.0.1:11814/ -d "cmd=insert&name=local_test_cs.local_test_cl&insertor={age:11,name:"Tom",male:true,parents:null,tel:[123456,654321],addr:{city:"Shenzhen"}}"
@expectation:	1 varCL.count(): 1
				2 check _id field: exist
				3 varCL.find({age:11,name:"Tom",male:true,parents:null,tel:[123456,654321],addr:{city:"Shenzhen"}})
			    	   return only 1 record
@modify list:
            	2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME;

function insertAndCheck ()
{
	var num = 1;
	var word = "insert";
	tryCatch(
		["cmd=" + word, "name=" + csName + '.' + clName, 'insertor={age:11,name:"Tom",male:true,parents:null,tel:[123456,654321],addr:{city:"Shenzhen"}}'],
		[0],
		"insertAndCheck: fail to run rest cmd=" + word );

	try
	{
		var size = varCL.count();
		if( num != size )
		{
			throw "insert " + num + "record by rest command, but cl.count() result is " + size;
		}
	}
	catch( e )
	{
		throw e;
	}

	try
	{
		var size = 0;
		var fieldNum = 0;
		var rc = varCL.find( { age: 11, name: "Tom", male: true, parents: null, tel: [123456, 654321], addr: { city: "Shenzhen" } } );
		while( rc.next() )
		{
			fieldNum = 0;
			if( rc.current().toObj()["_id"] == undefined )
			{
				throw "there is not _id field in record inserted by rest commad";
			}
			for( var x in rc.current().toObj() )
			{
				fieldNum++;
			}
			if( fieldNum != 7 )
			{
				throw "field number of the record inserted by rest command is 7, but cl.find() result is " + fieldNum;
			}
			size++;
		}
		if( num != size )
		{
			throw "insert " + num + "record by rest commad, but cl.find() result is " + size;
		}
	}
	catch( e )
	{
		throw e;
	}
}

commDropCL( db, csName, clName, true, true, "drop cl in begin" );
var opt = { ReplSize: 0 };
var varCL = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );
insertAndCheck();
commDropCL( db, csName, clName, false, false, "drop cl in clean" );