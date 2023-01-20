#ifndef SPT_DB_SCHEMA_HPP
#define SPT_DB_SCHEMA_HPP

#include "client.hpp"
#include "sptApi.hpp"

using sdbclient::sdbSchema ;
using sdbclient::_sdbSchema ;

namespace engine
{
   #define SPT_SCHEMA_NAME_FIELD            "_name"

   class _sptDBSchema : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBSchema ) ;
   public:
      _sptDBSchema( _sdbSchema *pSchema = NULL ) ;
      ~_sptDBSchema() ;

   public:
      INT32 construct( const _sptArguments &arg, _sptReturnVal &val, bson::BSONObj &detail ) ;
      INT32 destruct() ;

      INT32 addColumn( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 alterColumn( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 renameColumn( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 dropColumn( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 dropColumnDefault( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 setAttributes( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;
      INT32 alter( const _sptArguments &arg, _sptReturnVal &rval, bson::BSONObj &detail ) ;

   private:
      sdbSchema _schema ;
   } ;
   typedef _sptDBSchema sptDBSchema ;
}

#endif /* SPT_DB_SCHEMA_HPP */
