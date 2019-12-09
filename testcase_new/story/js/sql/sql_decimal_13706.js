/************************************
*@Description: 内置SQL支持decimal带条件查询
*@author:      luweikang
*@createdate:  2017.12.20
*@testlinkCase:seqDB-13706
**************************************/

main();

function main ()
{
   var csName = CHANGEDPREFIX + "_13706_CS";
   var clName = CHANGEDPREFIX + "_13706_CL";

   commDropCS( db, csName, true, "drop cs in the begin" );
   var cl = commCreateCL( db, csName, clName, null, null, true, false, "create cl in the begin" );

   println( "---begin test---" );
   isnotnullSQL( cl, csName, clName );

   insertSQL( db, cl, csName, clName, 'decimal(2147483648.2147483648)', { $decimal: "2147483648.2147483648" }, true );
   insertSQL( db, cl, csName, clName, 'decimal(1.7E+309)', { $decimal: "1.7E+309" }, true );
   insertSQL( db, cl, csName, clName, 'decimal(\"100.01\")', { $decimal: "100.01" }, false );

   updateSQL( db, cl, csName, clName, 'decimal(2147483648.2147483648)', 'decimal(\"9223372036854775808\")', { $decimal: "9223372036854775808" }, false );
   updateSQL( db, cl, csName, clName, 'decimal(2147483648.2147483648)', 'decimal(9223372036854775808)', { $decimal: "9223372036854775808" }, true );
   updateSQL( db, cl, csName, clName, 'decimal(1.7E+309)', 'decimal(100.01)', { $decimal: "100.01" }, true );

   selectSQL( db, csName, clName, 'decimal(\"9223372036854775808\")', false );
   selectSQL( db, csName, clName, 'decimal(9223372036854775808)', true );
   selectSQL( db, csName, clName, 'decimal(100.01)', true );

   deleteSQL( db, cl, csName, clName, 'decimal(\"9223372036854775808\")', { $decimal: "9223372036854775808" }, false );
   deleteSQL( db, cl, csName, clName, 'decimal(9223372036854775808)', { $decimal: "9223372036854775808" }, true );
   deleteSQL( db, cl, csName, clName, 'decimal(100.01)', { $decimal: "100.01" }, true );

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
      var sql = 'update ' + csName + "." + clName + ' set num=10 where textFields is not null';
      db.execUpdate( sql );
      var cursor = cl.find( { num: 10 } );
      if( cursor.next() == null )
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
         if( e != -6 && e != -195 )
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
         if( e != -6 && e != -195 )
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
         if( e != -6 && e != -195 )
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
         if( e != -6 && e != -195 )
         {
            throw buildException( "deleteSQL()", e, "delete record", '-6', e );
         }
      }
   }
}













