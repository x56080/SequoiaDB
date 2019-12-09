/******************************************************************************
*@Description: test insert data with options
*@author:      wangkexin
*@createdate:  2018.11.26
*@testlinkCase: seqDB-16714:options取值验证
******************************************************************************/

main( db );
function main ( db )
{
	var clName = CHANGEDPREFIX + "_insert16714";
	var oid = "5bf7575bdc4e88fa3dd16714";

	commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );

	var cs = db.getCS( COMMCSNAME );
	var cl = cs.createCL( clName, { ReplSize: 0, Compressed: true } );

	println( "begin to insert  data and check result" )
	// test a : check ReturnOID and ContOnDup default values
	var obj1 = { _id: 1, a: 1, b: 1 };
	cl.insert( obj1 );
	if( cl.count( obj1 ) != 1 )
	{
		throw "obj1 verify failed";
	}

	// test b : ReturnOID is true
	var obj2 = { "_id": oid, "test": "test16714" };
	var returnOidString = cl.insert( obj2, { ReturnOID: true } );
	if( returnOidString.toObj()._id.toString() != oid )
	{
		throw "returned oid is incorrect"
	}
	if( cl.count( obj2 ) != 1 )
	{
		throw "obj2 verify failed";
	}

	//test c : ReturnOID is false
	var obj3 = { "name": "Tom", "age": "20" };
	var returnOidNull = cl.insert( obj3, { ReturnOID: false } );
	if( returnOidNull !== undefined && returnOidNull.toObj()["_id"] !== undefined )
	{
		throw "obj3 verify failed, ReturnOID is false but returned oid is not undefined";
	}
	if( cl.count( obj3 ) != 1 )
	{
		throw "obj3 verify failed";
	}

	//test d : ContOnDup is true
	var obj4 = { "_id": 123 };
	cl.insert( [{ "_id": oid, test: "test16714_1" }, obj4], { ContOnDup: true } );
	if( cl.count( { "_id": oid } ) != 1 )
	{
		throw "obj4 verify failed, contOnDup is true ,but duplicate data is inserted";
	}
	if( cl.count( obj4 ) != 1 )
	{
		throw "obj4 verify failed";
	}

	//test e : ContOnDup is false
	try
	{
		cl.insert( obj1, { ContOnDup: false } );
		throw "exp fail but found success"
	} catch( e )
	{
		if( e != -38 )
		{
			throw buildException( "check insert data with options teste", e );
		}
	}
	println( "successful insertion of data with options" );
	commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the end , error" );
}

