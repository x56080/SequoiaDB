/****************************************************************************
@Description : Creating index and get the index .
@Modify list :
               2014-5-18  xiaojun Hu  Modify
****************************************************************************/
try
{
   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   var optionObj = { ReplSize: 0, Compressed: true };
   var varCL = commCreateCLByOption( db, csName, clName, optionObj, true,
      false, "create collecton 1 failed" );
}
catch( e )
{
   println( "Failed to create CS and CL, rc=" + e );
   throw e;
}

try
{
   varCL.createIndex( "testindex", { a: 1 }, true );
   varCL.createIndex( "nameIndex", { "name": -1 }, true, false );
}
catch( e )
{
   println( "failed to create index, rc=" + e );
   throw e;
}

try
{
   varCL.insert( { a: 1, "name": "hihao" } );
}
catch( e )
{
   println( "failed to insert record, rc=" + e );
   throw e;
}

var index;
try
{
   index = varCL.getIndex( "testindex" );
}
catch( e )
{
   println( "failed to get index, rc=" + e );
   throw e;
}

index = index.toString();
index = eval( '(' + index + ')' );
var _index = index["IndexDef"];
//var _index = eval("("+_index+")") ;
if( "testindex" != _index["name"] )
{
   println( "wrong index name:" + _index["name"] );
   throw "ErrWrongIndex";
}

//_index = eval("("+index["key"]+")") ;
if( 1 != _index["key"]["a"] )
{
   println( "wrong index def:" + index["key"] );
   throw "ErrIdxDef";
}
if( true != _index["unique"] )
{
   println( "wrong index unique" );
   throw "ErrIdxUnique";
}

var jx = 0;
try
{
   varCL.insert( { a: 1, "name": "hihao1232" } );
}
catch( e )
{
   if( -38 != e )
   {
      println( "Failed to inset data to database after create index, rc=" + e );
      throw e;
   }
}


try
{
   commDropCL( db, csName, clName, false, false,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

