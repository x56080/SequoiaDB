
const MongoClient = require( 'mongodb' ).MongoClient;

const url = 'mongodb://localhost:11817/mydb?readPreference=secondaryPreferred';

MongoClient.connect( url, ( err, client ) =>
{
   if( err )
   {
      console.error( 'Database connection error:', err );
      return;
   }

   const db = client.db( 'db_nodejs_readRef' );
   const collection = db.collection( 'cl_readRef' );

   // 插入数据
   const dataToInsert = { name: 'John', age: 30 };
   collection.insertOne( dataToInsert, ( insertErr, result ) =>
   {
      if( insertErr )
      {
         console.error( 'Insertion error:', insertErr );
      } else
      {
         console.log( 'Data inserted:', result.ops );
      }

      // 查询数据
      collection.find( { name: 'John' } ).toArray( ( queryErr, documents ) =>
      {
         if( queryErr )
         {
            console.error( 'Query error:', queryErr );
         } else
         {
            console.log( 'Queried documents:', documents );
         }

         // 关闭连接
         client.close();
      } );
   } );
} );
