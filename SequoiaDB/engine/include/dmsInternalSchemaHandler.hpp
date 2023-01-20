#ifndef DMS_INTSCHEMA_HANDLER_HPP__
#define DMS_INTSCHEMA_HANDLER_HPP__

#include "dmsEventHandler.hpp"
#include "dmsSysSUMgr.hpp"

namespace engine
{
   class _dmsInternalSchemaHandler : public _IDmsEventHandler
   {
         struct cmp_column_name
         {
            bool operator()( const CHAR *l, const CHAR *r)
            {
               return ossStrcmp( l, r ) < 0 ;
            }
         } ;
      public:
         _dmsInternalSchemaHandler() ;
         virtual ~_dmsInternalSchemaHandler() {}

         virtual INT32 onCreateIndex ( IDmsEventHolder *pEventHolder,
                                       IDmsSUCacheHolder *pCacheHolder,
                                       const dmsEventCLItem &clItem,
                                       const dmsEventIdxItem &idxItem,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropIndex ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     const dmsEventCLItem &clItem,
                                     const dmsEventIdxItem &idxItem,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;

         OSS_INLINE virtual UINT32 getMask () const
         {
            return DMS_EVENT_MASK_SCHEMA ;
         }

         OSS_INLINE virtual const CHAR *getName() const
         {
            return "internal schema handler" ;
         }

   } ;
   typedef _dmsInternalSchemaHandler dmsInternalSchemaHandler ;

}

#endif /* DMS_INTSCHEMA_HANDLER_HPP__ */
