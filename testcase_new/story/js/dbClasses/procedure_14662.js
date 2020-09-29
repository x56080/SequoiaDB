/*******************************************************************
* @Description : create procedure and eval to return
*                Sdb SdbCS SdbCollection SdbCursor SdbQuery
*                SdbReplicaGroup SdbNode SdbDomain CLCount
*                BinData ObjectId Timestamp Regex MinKey MaxKey
*                NumberLong SdbDate
*                seqDB-14662:执行存储过程返回db/cs/cl等对象
* @author      : Liang XueWang
*                2018-03-10
*******************************************************************/
var clName = COMMCLNAME + "_dbClasses14662";
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
;

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }

   commCreateCL( db, COMMCSNAME, clName );

   evalSdb( db ); // can't return Sdb
   evalSdbCS( db );
   evalSdbCollection( db );
   evalSdbCursor( db );
   evalSdbQuery( db );
   evalSdbReplicaGroup( db );
   evalSdbNode( db );
   evalSdbDomain( db );
   evalCLCount( db );
   evalBinData( db );
   evalObjectId( db );
   evalTimestamp( db );
   evalRegex( db );
   evalMinKey( db );
   evalMaxKey( db );
   evalNumberLong( db );
   evalSdbDate( db );

   commDropCL( db, COMMCSNAME, clName );
}

function evalSdb ( db )
{
   commCreateProcedure( db, function getSdb ( host, svc )
   {
      var sdb = new Sdb( host, svc );
      return sdb;
   } );
   try
   {
      var sdb = db.eval( "getSdb( \"" + COORDHOSTNAME + "\", \"" + COORDSVCNAME + "\" )" );
      throw 0;
   }
   catch( e )
   {
      if( e !== -10 )
      {
         throw new Error( e );
      }
   }
   commRemoveProcedure( db, "getSdb" );
}

function evalSdbCS ( db )
{
   commCreateProcedure( db, function getSdbCS ( host, svc, csname )
   {
      var sdb = new Sdb( host, svc );
      return sdb.getCS( csname );
   } );
   try
   {
      var cs = db.eval( "getSdbCS( \"" + COORDHOSTNAME + "\", \"" + COORDSVCNAME +
         "\", \"" + COMMCSNAME + "\" )" );
      println( "cs instanceof SdbCS: " + ( cs instanceof SdbCS ) );
      cs.toString();
      var cl = cs.getCL( clName );
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getSdbCS" );

}

function evalSdbCollection ( db )
{
   commCreateProcedure( db, function getSdbCollection ( host, svc, csname, clname )
   {
      var sdb = new Sdb( host, svc );
      return sdb.getCS( csname ).getCL( clname );
   } );
   try
   {
      var cl = db.eval( "getSdbCollection( \"" + COORDHOSTNAME + "\", \"" + COORDSVCNAME +
         "\", \"" + COMMCSNAME + "\", \"" + clName + "\" )" );
      println( "cl instanceof SdbCollection: " + ( cl instanceof SdbCollection ) );
      // cl.toString();
      cl.find();
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getSdbCollection" );

}

function evalSdbCursor ( db )
{
   commCreateProcedure( db, function getSdbCursor ( host, svc, csname, clname )
   {
      var sdb = new Sdb( host, svc );
      var collection = db.getCS( csname ).getCL( clname );
      return collection.find().explain( { Run: true } );
   } );
   try
   {
      var cursor = db.eval( "getSdbCursor( \"" + COORDHOSTNAME + "\", " +
         "\"" + COORDSVCNAME + "\", \"" + COMMCSNAME + "\", \"" +
         clName + "\" )" );
      println( "cursor instanceof SdbCursor: " + ( cursor instanceof SdbCursor ) );
      cursor.next();
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getSdbCursor" );

}

function evalSdbQuery ( db )
{
   commCreateProcedure( db, function getSdbQuery ( host, svc, csname, clname )
   {
      var sdb = new Sdb( host, svc );
      var collection = db.getCS( csname ).getCL( clname );
      return collection.find();
   } );
   try
   {
      var query = db.eval( "getSdbQuery( \"" + COORDHOSTNAME + "\", " +
         "\"" + COORDSVCNAME + "\", \"" + COMMCSNAME + "\", \"" +
         clName + "\" )" );
      println( "query instanceof SdbQuery: " + ( query instanceof SdbQuery ) ); // SdbCursor
      query.size();
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getSdbQuery" );

}

function evalSdbReplicaGroup ( db )
{
   commCreateProcedure( db, function getSdbReplicaGroup ( host, svc, rgname )
   {
      var sdb = new Sdb( host, svc );
      return sdb.getRG( rgname );
   } );
   try
   {
      var rg = db.eval( "getSdbReplicaGroup( \"" + COORDHOSTNAME + "\", " +
         "\"" + COORDSVCNAME + "\", \"SYSCoord\" )" );
      println( "rg instanceof SdbReplicaGroup: " + ( rg instanceof SdbReplicaGroup ) );
      rg.getDetail();
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getSdbReplicaGroup" );

}

function evalSdbNode ( db )
{
   commCreateProcedure( db, function getSdbNode ( host, svc, rgname )
   {
      var sdb = new Sdb( host, svc );
      var rg = sdb.getRG( rgname );
      return rg.getSlave();
   } );

   try
   {
      var node = db.eval( "getSdbNode( \"" + COORDHOSTNAME + "\", " +
         "\"" + COORDSVCNAME + "\", \"SYSCoord\" )" );
      println( "node instanceof SdbNode: " + ( node instanceof SdbNode ) );
      node.getHostName();
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getSdbNode" );

}

function evalSdbDomain ( db )
{
   var groups = commGetDataGroupNames( db );
   var domainName = "testDomain14662";
   commDropDomain( db, domainName );
   commCreateDomain( db, domainName, groups );

   commCreateProcedure( db, function getSdbDomain ( host, svc, domainname )
   {
      var sdb = new Sdb( host, svc );
      return sdb.getDomain( domainname );
   } );
   try
   {
      var domain = db.eval( "getSdbDomain( \"" + COORDHOSTNAME + "\", " +
         "\"" + COORDSVCNAME + "\", \"" + domainName + "\" )" );
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getSdbDomain" );
   commDropDomain( db, domainName );
}

function evalCLCount ( db )
{
   commCreateProcedure( db, function getCLCount ( host, svc, csname, clname )
   {
      var sdb = new Sdb( host, svc );
      var cl = sdb.getCS( csname ).getCL( clname );
      return cl.count();
   } );

   try
   {
      var cnt = db.eval( "getCLCount( \"" + COORDHOSTNAME + "\", " +
         "\"" + COORDSVCNAME + "\", \"" + COMMCSNAME +
         "\", \"" + clName + "\" )" );
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getCLCount" );

}

function evalBinData ( db )
{
   commCreateProcedure( db, function getBinData ( data, type )
   {
      return BinData( data, type );
   } );
   try
   {
      var data = "aGVsbG8gd29ybGQ";
      var type = "1";
      var bindata = db.eval( "getBinData( \"" + data + "\", \"" + type + "\" )" );
      println( "bindata instanceof BinData: " + ( bindata instanceof BinData ) );
      var expectval = BinData( data, type );
      if( bindata.toString() !== expectval.toString() )
      {
         throw "expect: " + expectval + "actual: " + bindata;
      }
   }
   catch( e )
   {
      throw new Error( e );
   }

   commRemoveProcedure( db, "getBinData" );

}

function evalObjectId ( db )
{
   commCreateProcedure( db, function getObjectId ( data )
   {
      return ObjectId( data );
   } );
   try
   {
      var data = "55713f7953e6769804000001";
      var oid = db.eval( "getObjectId( \"" + data + "\" )" );
      println( "oid instanceof ObjectId: " + ( oid instanceof ObjectId ) );
      var expectval = ObjectId( data );
      if( oid.toString() !== expectval.toString() )
      {
         throw "expect: " + expectval + "actual: " + oid;
      }
   }
   catch( e )
   {
      throw new Error( e );
   }

   commRemoveProcedure( db, "getObjectId" );
}

function evalTimestamp ( db )
{
   commCreateProcedure( db, function getTimestamp ( time )
   {
      return Timestamp( time );
   } );

   try
   {
      var time = "2015-06-05-16.10.33.000000";
      var timestamp = db.eval( "getTimestamp( \"" + time + "\" )" );
      println( "timestamp instanceof Timestamp: " + ( timestamp instanceof Timestamp ) );
      var expectval = Timestamp( time );
      if( timestamp.toString() !== expectval.toString() )
      {
         throw "expect: " + expectval + "actual: " + timestamp;
      }
   }
   catch( e )
   {
      throw new Error( e );
   }

   commRemoveProcedure( db, "getTimestamp" );
}

function evalRegex ( db )
{
   commCreateProcedure( db, function getRegex ( pattern, options )
   {
      return Regex( pattern, options );
   } );
   try
   {
      var pattern = "^W";
      var options = "i";
      var regex = db.eval( "getRegex( \"" + pattern + "\", \"" + options + "\" )" );
      println( "regex instanceof Regex: " + ( regex instanceof Regex ) );
      var expectval = Regex( pattern, options );
      if( regex.toString() !== expectval.toString() )
      {
         throw "expect: " + expectval + "actual: " + regex;
      }

   }
   catch( e )
   {
      throw new Error( e );
   }

   commRemoveProcedure( db, "getRegex" );
}

function evalMinKey ( db )
{
   commCreateProcedure( db, function getMinKey ()
   {
      return MinKey();
   } );
   try
   {
      var minKey = db.eval( "getMinKey()" );
      println( "minKey instanceof MinKey: " + ( minKey instanceof MinKey ) );
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getMinKey" );
}

function evalMaxKey ( db )
{
   commCreateProcedure( db, function getMaxKey ()
   {
      return MaxKey();
   } );
   try
   {
      var maxKey = db.eval( "getMaxKey()" );
      println( "maxKey instanceof MaxKey: " + ( maxKey instanceof MaxKey ) );
   }
   catch( e )
   {
      throw new Error( e );
   }
   commRemoveProcedure( db, "getMaxKey" );

}

function evalNumberLong ( db )
{
   commCreateProcedure( db, function getNumberLong ( number )
   {
      return NumberLong( number );
   } );
   try
   {
      var number = 2147483648;
      var numberLong = db.eval( "getNumberLong( " + number + " )" );
      println( "numberLong instanceof NumberLong: " + ( numberLong instanceof NumberLong ) );
      var expectval = NumberLong( number );
      if( numberLong.toString() !== expectval.toString() )
      {
         throw "expect: " + expectval + "actual: " + numberLong;
      }
   }
   catch( e )
   {
      throw new Error( e );
   }

   commRemoveProcedure( db, "getNumberLong" );
}

function evalSdbDate ( db )
{
   commCreateProcedure( db, function getSdbDate ( date )
   {
      return SdbDate( date );
   } );
   try
   {
      var date = "2015-03-13";
      var sdbDate = db.eval( "getSdbDate( \"" + date + "\" )" );
      println( "sdbDate instanceof SdbDate: " + ( sdbDate instanceof SdbDate ) );
      var expectval = SdbDate( date );
      if( sdbDate.toString() !== expectval.toString() )
      {
         throw "expect: " + date + "actual: " + sdbDate;
      }
   }
   catch( e )
   {
      throw new Error( e );
   }

   commRemoveProcedure( db, "getSdbDate" );
}

/******************************************************************************
@description  create procedure
@author  zhaoyu
@parameter
***************************************************************************** */
function commCreateProcedure ( db, code, ignoreExisted )
{
   if( ignoreExisted == undefined ) { ignoreExisted = false; }
   try
   {
      db.createProcedure( code );
   } catch( e )
   {
      if( !commCompareErrorCode( e, -342 ) || !ignoreExisted )
      {
         commThrowError( e, "commCreateProcedure, create procedure: " + code + " failed: " + e );
      }
   }
}

/******************************************************************************
@description  remove procedure
@author  zhaoyu
@parameter
***************************************************************************** */
function commRemoveProcedure ( db, functionName, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   try
   {
      db.removeProcedure( functionName );
   } catch( e )
   {
      if( !commCompareErrorCode( e, -233 ) || !ignoreNotExist )
      {
         commThrowError( e, "commRemoveProcedure, remove procedure: " + functionName + " failed: " + e );
      }
   }
}