/******************************************************************************
@Description : 1. regex related query explain [jira_330]
@Modify list :
               2014-10-28 wuyida  Init
******************************************************************************/

CHANGEDPREFIX_IDX = CHANGEDPREFIX + "_idx";

if( false == commIsStandalone( db ) )
{
   // set session get data from master
   db.setSessionAttr( { "PreferedInstance": "M" } );
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

//create CS
try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
}
catch( e )
{
   println( "failed to create cs,rc=" + e );
   throw e;
}

//create CL
try
{
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );
   println( "create CL finished" );
} catch( e )
{
   //collection already exist,use it
   if( e != -22 )
   {
      println( "can't create CL:" + COMMCLNAME + " rc=" + e );
      throw e;
   }
   else
   {
      varCL = varCS.getCL( COMMCLNAME );
      varCL.remove();
      println( "use CL:" + COMMCLNAME );
   }
}

//insert data
try
{
   varCL.insert( { a: 'abcdefgh', b: 0 } );
   varCL.insert( { a: 'ABCDefg', b: 1 } );
   varCL.insert( { a: ['123', '321', '213'], b: 2 } );
   varCL.insert( { a: 'zuilkj', b: -1 } );
   varCL.insert( { a: 4, b: 4 } );
} catch( e )
{
   println( "insert data fail! rc=" + e );
   throw e;
}
println( "insert data finished" );

//create index
try
{
   varCL.createIndex( CHANGEDPREFIX_IDX, { a: 1 } );
   println( "create index finished" );
} catch( e )
{
   //when redefine index, ignore the exception
   if( e != -247 )
   {
      println( "can't create index:" + CHANGEDPREFIX_IDX + " rc=" + e );
      throw e;
   }
   println( "already exist index:" + CHANGEDPREFIX_IDX );
}

//find({a:{$regex:'^abc',$options:'xi'}}).explain()
try
{
   var sel = varCL.find( { a: { $regex: '^abc', $options: 'xi' } } ).explain();
   var size = 0;
   var exprows = 1;
   var flag = true;
   while( sel.next() )
   {
      size++;
      if( size > exprows )
      {
         flag = false;
         throw 1;
      }
      var ret = sel.current();
      //expected result:ScanType is tbscan
      if( ret.toObj()['ScanType'] != 'tbscan' )
      {
         flag = false;
         throw 2;
      }
   }
   sel.close();
   if( flag && size != exprows )
   {
      flag = false;
      throw 1;
   }
} catch( e )
{
   if( e != 1 && e != 2 )
   {
      println( "select data fail! rc=" + e + ", sel is: " + sel );
      throw e;
   }
   else if( e == 1 )
   {
      println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
      throw e;
   }
   else if( e == 2 )
   {
      println( "return incorrect record! expected:{ScanType:'tbscan'} returned:" + ret );
      throw e;
   }
}
println( "select regex options with 'i' use tbscan finished!" );

//find({a:{$regex:'^abc',$options:'x'}}).explain()
try
{
   var sel = varCL.find( { a: { $regex: '^abc', $options: 'x' } } ).explain();
   var size = 0;
   var exprows = 1;
   var flag = true;
   while( sel.next() )
   {
      size++;
      if( size > exprows )
      {
         flag = false;
         throw 1;
      }
      var ret = sel.current();
      //expected result:ScanType is ixscan
      if( ret.toObj()['ScanType'] != 'ixscan' )
      {
         flag = false;
         throw 2;
      }
   }
   sel.close();
   if( flag && size != exprows )
   {
      flag = false;
      throw 1;
   }
} catch( e )
{
   if( e != 1 && e != 2 )
   {
      println( "select data fail! rc=" + e );
      throw e;
   }
   else if( e == 1 )
   {
      println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
      throw e;
   }
   else if( e == 2 )
   {
      println( "return incorrect record! expected:{ScanType:'ixscan'} returned:" + ret );
      throw e;
   }
}
println( "select regex options without 'i' use ixscan finished!" );

//find({a:{$regex:'^abc'}}).explain()
try
{
   var sel = varCL.find( { a: { $regex: '^abc' } } ).explain();
   var size = 0;
   var exprows = 1;
   var flag = true;
   while( sel.next() )
   {
      size++;
      if( size > exprows )
      {
         flag = false;
         throw 1;
      }
      var ret = sel.current();
      //expected result:ScanType is ixscan
      if( ret.toObj()['ScanType'] != 'ixscan' )
      {
         flag = false;
         throw 2;
      }
   }
   sel.close();
   if( flag && size != exprows )
   {
      flag = false;
      throw 1;
   }
} catch( e )
{
   if( e != 1 && e != 2 )
   {
      println( "select data fail! rc=" + e );
      throw e;
   }
   else if( e == 1 )
   {
      println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
      throw e;
   }
   else if( e == 2 )
   {
      println( "return incorrect record! expected:{ScanType:'ixscan'} returned:" + ret );
      throw e;
   }
}
println( "select regex fixed start string use ixscan finished!" );

//find({a:{$regex:'abc'}}).explain()
try
{
   var sel = varCL.find( { a: { $regex: 'abc' } } ).explain();
   var size = 0;
   var exprows = 1;
   var flag = true;
   while( sel.next() )
   {
      size++;
      if( size > exprows )
      {
         flag = false;
         throw 1;
      }
      var ret = sel.current();
      //expected result:ScanType is tbscan
      if( ret.toObj()['ScanType'] != 'tbscan' )
      {
         flag = false;
         throw 2;
      }
   }
   sel.close();
   if( flag && size != exprows )
   {
      flag = false;
      throw 1;
   }
} catch( e )
{
   if( e != 1 && e != 2 )
   {
      println( "select data fail! rc=" + e );
      throw e;
   }
   else if( e == 1 )
   {
      println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
      throw e;
   }
   else if( e == 2 )
   {
      println( "return incorrect record! expected:{ScanType:'tbscan'} returned:" + ret );
      throw e;
   }
}
println( "select regex no fixed start string use tbscan finished!" );

//find({$or:[a:{$regex:'abc'},a:{$gt:'abcd'}]}).explain()
try
{
   var sel = varCL.find( { $or: [{ a: { $regex: '^abc' } }, { a: { $gt: 'abcd' } }] } ).explain();
   var size = 0;
   var exprows = 1;
   var flag = true;
   while( sel.next() )
   {
      size++;
      if( size > exprows )
      {
         flag = false;
         throw 1;
      }
      var ret = sel.current();
      //expected result:ScanType is tbscan
      if( ret.toObj()['ScanType'] != 'tbscan' )
      {
         flag = false;
         throw 2;
      }
   }
   sel.close();
   if( flag && size != exprows )
   {
      flag = false;
      throw 1;
   }
} catch( e )
{
   if( e != 1 && e != 2 )
   {
      println( "select data fail! rc=" + e );
      throw e;
   }
   else if( e == 1 )
   {
      println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
      throw e;
   }
   else if( e == 2 )
   {
      println( "return incorrect record! expected:{ScanType:'tbscan'} returned:" + ret );
      throw e;
   }
}
println( "select regex with $or use tbscan finished!" );

//find({$and:[a:{$regex:'abc'},a:{$gt:'abcd'}]}).explain()
try
{
   var sel = varCL.find( { $and: [{ a: { $regex: '^abc' } }, { a: { $gt: 'abcd' } }] } ).explain();
   var size = 0;
   var exprows = 1;
   var flag = true;
   while( sel.next() )
   {
      size++;
      if( size > exprows )
      {
         flag = false;
         throw 1;
      }
      var ret = sel.current();
      //expected result:ScanType is ixscan
      if( ret.toObj()['ScanType'] != 'ixscan' )
      {
         flag = false;
         throw 2;
      }
   }
   sel.close();
   if( flag && size != exprows )
   {
      flag = false;
      throw 1;
   }
} catch( e )
{
   if( e != 1 && e != 2 )
   {
      println( "select data fail! rc=" + e );
      throw e;
   }
   else if( e == 1 )
   {
      println( "return rows not expected! expected:" + exprows + " return:" + size + ( size > exprows ? " or more" : "" ) );
      throw e;
   }
   else if( e == 2 )
   {
      println( "return incorrect record! expected:{ScanType:'ixscan'} returned:" + ret );
      throw e;
   }
}
println( "select regex with $and use ixscan finished!" );

//clean test-env
try
{
   varCL.dropIndex( CHANGEDPREFIX_IDX );
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop cl in the end" );
} catch( e )
{
   println( "clean test-evn fail! rc=" + e );
   throw e;
}
println( "clean test-evn succ!" );

