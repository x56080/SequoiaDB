/******************************************************************************
*@Description:  test insert data with options
*@author:       wangkexin
*@createdate:   2018.11.26
*@testlinkCase: seqDB-16714:options取值验证
******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_16714";
   var oid = "5bf7575bdc4e88fa3dd16714";
   var cl = readyCL( clName );

   // test a : check ReturnOID and ContOnDup default values
   var obj1 = { _id: 1, a: 1, b: 1 };
   var defaultReturn = cl.insert( obj1 );
   if( defaultReturn.toObj().InsertedNum !== 1 || defaultReturn.toObj().DuplicatedNum !== 0 )
   {
      throw new Error( "defaultReturn: " + defaultReturn );
   }

   // test b : ReturnOID is true
   var obj2 = { "_id": oid, "test": "test16714" };
   var returnOidString = cl.insert( obj2, { ReturnOID: true } );
   if( returnOidString.toObj()._id.toString() != oid )
   {
      throw new Error( "returnOidString.toObj()._id.toString(): " + returnOidString.toObj()._id.toString() );
   }

   //test c : ReturnOID is false
   var obj3 = { "name": "Tom", "age": "20" };
   var returnOidNull = cl.insert( obj3, { ReturnOID: false } );
   if( returnOidNull === undefined || returnOidNull.toObj()._id !== undefined )
   {
      throw new Error( "returnOidNull: " + returnOidNull );
   }

   //test d : ContOnDup is true
   var obj4 = { "_id": 123 };
   cl.insert( [{ "_id": oid, test: "test16714_1" }, obj4], { ContOnDup: true } );
   var cursor = cl.find( { "_id": oid } );
   commCompareResults( cursor, [{ "_id": oid, test: "test16714" }], false );
   cursor = cl.find( obj4 );
   commCompareResults( cursor, [obj4], false )

   //test e : ContOnDup is false
   try
   {
      cl.insert( obj1, { ContOnDup: false } );
      throw new Error( "need throw error" );
   } catch( e )
   {
      if( e.message != -38 )
      {
         throw e;
      }
   }
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the end , error" );
}

