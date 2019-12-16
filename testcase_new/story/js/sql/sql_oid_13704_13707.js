/************************************
*@Description: 内置SQL支持oid带条件查询,支持is not null查询
*@author:      luweikang
*@createdate:  2017.12.20
*@testlinkCase:seqDB-13704，seqDB-13707
**************************************/

main();

function main ()
{
   var csName = CHANGEDPREFIX + "_13704_CS";
   var clName = CHANGEDPREFIX + "_13704_CL";

   commDropCS( db, csName, true, "drop cs in the begin" );
   var cl = commCreateCL( db, csName, clName, {}, true, false, "create cl in the begin" );

   println( "---begin test---" );
   isnotnullSQL( cl, csName, clName );

   insertSQL( db, cl, csName, clName, 'oid(\"55713F7953E6769804000001\")', { $oid: "55713f7953e6769804000001" }, true );
   insertSQL( db, cl, csName, clName, 'oid(\"55713f7953e\")', { $oid: "55713f7953e6769804000001" }, false );

   updateSQL( db, cl, csName, clName, 'oid(\"55713f7953e6769804000001\")', 'oid(\"55713f7953e\")', null, false );
   updateSQL( db, cl, csName, clName, 'oid(\"55713F7953E6769804000001\")', 'oid(\"55713f7953e6769804000111\")', { $oid: "55713f7953e6769804000111" }, true );

   selectSQL( db, csName, clName, 'oid(\"55713f7953e6769804000111\")', true );
   selectSQL( db, csName, clName, 'oid(\"55713f7953e\")', false );

   deleteSQL( db, cl, csName, clName, 'oid(\"55713f7953e\")', null, false );
   deleteSQL( db, cl, csName, clName, 'oid(\"55713F7953E6769804000111\")', { $oid: "55713f7953e6769804000111" }, true );

   println( "---end the test---" );
   commDropCS( db, csName, true, "drop CS in the end" );
}

function isnotnullSQL ( cl, csName, clName )
{
   var doc = [{ num: 1, textFields: null },
   { num: 2, textFields: 'textstr' }];
   try
   {

      cl.insert( doc );
      var sql = 'select * from ' + csName + "." + clName + ' where textFields is not null';
      var cursor = db.exec( sql );
      if( cursor.next() != null )
      {
         var obj = cursor.current().toObj();
         var num = obj.num;
         if( num !== 2 )
         {
            throw buildException( "isnotnullSQL()", null, "check record", "{num:2, textFields:'textstr'}", obj );
         }
      }
      else
      {
         throw buildException( "isnotnullSQL()", null, "check record", "have data", "no data" );
      }
   }
   catch( e )
   {
      throw buildException( "insertData()", e, "insert data", "insert success", "insert faild: " + e );
   }
}


function insertSQL ( db, cl, csName, clName, insertValue, checkValue, result )
{
   var sql = 'insert into ' + csName + '.' + clName + '(num, textFields ) values (3, ' + insertValue + ')';
   if( result )
   {
      try
      {
         db.execUpdate( sql );
         var cursor = cl.find( { textFields: checkValue } );
         if( cursor.next() == null )
         {
            throw buildException( "insertSQL()", null, "check record", "have data", "no data" );
         }
      }
      catch( e )
      {
         throw buildException( "insertSQL()", e, "insert record", "insert success", "insert faild: " + e );
      }
   }
   else
   {
      try
      {
         db.execUpdate( sql );
         throw buildException( "insertSQL()", null, "insert error record", "insert faild", "insert success" );
      }
      catch( e )
      {
         if( e != -6 )
         {
            throw buildException( "insertSQL()", e, "insert record", '-6', e );
         }
      }
   }

}

function updateSQL ( db, cl, csName, clName, oldValue, newValue, checkValue, result )
{
   var sql = 'update ' + csName + '.' + clName + ' set textFields=' + newValue + ' where textFields=' + oldValue;
   if( result )
   {
      try
      {
         db.execUpdate( sql );
         var cursor = cl.find( { textFields: checkValue } );
         if( cursor.next() == null )
         {
            throw buildException( "updateSQL()", null, "check record", "have data", "no data" );
         }
      }
      catch( e )
      {
         throw buildException( "updateSQL()", e, "update record", "update success", "update faild: " + e );
      }
   }
   else
   {
      try
      {
         db.execUpdate( sql );
         throw buildException( "updateSQL()", null, "update error record", "update faild", "update success" );
      }
      catch( e )
      {
         if( e != -6 )
         {
            throw buildException( "updateSQL()", e, "update record", '-6', e );
         }
      }
   }
}

function selectSQL ( db, csName, clName, value, result )
{
   var sql = 'select * from ' + csName + "." + clName + ' where textFields=' + value;
   if( result )
   {
      try
      {
         var cursor = db.exec( sql );
         if( cursor.next() == null )
         {
            throw buildException( "selectSQL()", null, "check record", "have data", "no data" );
         }
      }
      catch( e )
      {
         throw buildException( "selectSQL()", e, "select record", "select success", "select faild: " + e );
      }
   }
   else
   {
      try
      {
         db.execUpdate( sql );
         throw buildException( "selectSQL()", null, "select error record", "select faild", "select success" );
      }
      catch( e )
      {
         if( e != -6 )
         {
            throw buildException( "selectSQL()", e, "select record", '-6', e );
         }
      }
   }
}

function deleteSQL ( db, cl, csName, clName, deleteValue, checkValue, result )
{
   var sql = 'delete from ' + csName + '.' + clName + ' where textFields=' + deleteValue;
   if( result )
   {
      try
      {
         db.execUpdate( sql );
         var cursor = cl.find( { textFields: checkValue } );
         if( cursor.next() != null )
         {
            throw buildException( "deleteSQL()", null, "check record", "no data", "have data" );
         }
      }
      catch( e )
      {
         throw buildException( "deleteSQL()", e, "delete record", "delete success", "delete faild: " + e );
      }
   }
   else
   {
      try
      {
         db.execUpdate( sql );
         throw buildException( "deleteSQL()", null, "delete error record", "delete faild", "delete success" );
      }
      catch( e )
      {
         if( e != -6 )
         {
            throw buildException( "deleteSQL()", e, "delete record", '-6', e );
         }
      }
   }
}













