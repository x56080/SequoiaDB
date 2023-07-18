/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = rtnLogRecordSerializer.hpp

   Descriptive Name = Log Record Serializer

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOG_RECORD_SERIALIZER_HPP__
#define RTN_LOG_RECORD_SERIALIZER_HPP__

#include "dpsOp2Record.hpp"
#include "oss.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _rtnLogRecordSerializer define
    */
   // log record serializer
   class _rtnLogRecordSerializer : public SDBObject
   {
   public:
      _rtnLogRecordSerializer() = default ;
      ~_rtnLogRecordSerializer() = default ;

      static INT32 buildRecord( const dpsLogRecord &record,
                                bson::BSONObjBuilder &builder ) ;

   protected:
      static void _collectionSpaceToBSON( const CHAR *name,
                                          bson::BSONObjBuilder &builder ) ;
      static void _collectionToBSON( const CHAR *name,
                                     bson::BSONObjBuilder &builder ) ;
      static void _objectKeyToBSON( const bson::BSONObj &object,
                                    bson::BSONObjBuilder &builder ) ;
      static void _transIDToBSON( const DPS_TRANS_ID &transID,
                                  bson::BSONObjBuilder &builder ) ;
      static void _transInfoToBSON( const dpsRecordTransInfo &transInfo,
                                    bson::BSONObjBuilder &builder ) ;
      static void _timeInfoToBSON( const dpsRecordTimeInfo &timeInfo,
                                   bson::BSONObjBuilder &builder ) ;

      static INT32 _buildInsertRecord( const dpsLogRecord &record,
                                       bson::BSONObjBuilder &builder,
                                       dpsRecordTransInfo &transInfo,
                                       dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildUpdateRecord( const dpsLogRecord &record,
                                       bson::BSONObjBuilder &builder,
                                       dpsRecordTransInfo &transInfo,
                                       dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildDeleteRecord( const dpsLogRecord &record,
                                       bson::BSONObjBuilder &builder,
                                       dpsRecordTransInfo &transInfo,
                                       dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildPopRecord( const dpsLogRecord &record,
                                    bson::BSONObjBuilder &builder,
                                    dpsRecordTransInfo &transInfo,
                                    dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildCreateCSRecord( const dpsLogRecord &record,
                                         bson::BSONObjBuilder &builder,
                                         dpsRecordTransInfo &transInfo,
                                         dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildDeleteCSRecord( const dpsLogRecord &record,
                                         bson::BSONObjBuilder &builder,
                                         dpsRecordTransInfo &transInfo,
                                         dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildRenameCSRecord( const dpsLogRecord &record,
                                         bson::BSONObjBuilder &builder,
                                         dpsRecordTransInfo &transInfo,
                                         dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildCreateCLRecord( const dpsLogRecord &record,
                                         bson::BSONObjBuilder &builder,
                                         dpsRecordTransInfo &transInfo,
                                         dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildDeleteCLRecord( const dpsLogRecord &record,
                                         bson::BSONObjBuilder &builder,
                                         dpsRecordTransInfo &transInfo,
                                         dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildCreateIXRecord( const dpsLogRecord &record,
                                         bson::BSONObjBuilder &builder,
                                         dpsRecordTransInfo &transInfo,
                                         dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildDeleteIXRecord( const dpsLogRecord &record,
                                         bson::BSONObjBuilder &builder,
                                         dpsRecordTransInfo &transInfo,
                                         dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildRenameCLRecord( const dpsLogRecord &record,
                                         bson::BSONObjBuilder &builder,
                                         dpsRecordTransInfo &transInfo,
                                         dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildTruncateCLRecord( const dpsLogRecord &record,
                                           bson::BSONObjBuilder &builder,
                                           dpsRecordTransInfo &transInfo,
                                           dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildInvalidateCataRecord( const dpsLogRecord &record,
                                               bson::BSONObjBuilder &builder,
                                               dpsRecordTransInfo &transInfo,
                                               dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildTransCommitRecord( const dpsLogRecord &record,
                                            bson::BSONObjBuilder &builder,
                                            dpsRecordTransInfo &transInfo,
                                            dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildLobWriteRecord( const dpsLogRecord &record,
                                         bson::BSONObjBuilder &builder,
                                         dpsRecordTransInfo &transInfo,
                                         dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildLobRemoveRecord( const dpsLogRecord &record,
                                          bson::BSONObjBuilder &builder,
                                          dpsRecordTransInfo &transInfo,
                                          dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildLobUpdateRecord( const dpsLogRecord &record,
                                          bson::BSONObjBuilder &builder,
                                          dpsRecordTransInfo &transInfo,
                                          dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildLobTruncateRecord( const dpsLogRecord &record,
                                            bson::BSONObjBuilder &builder,
                                            dpsRecordTransInfo &transInfo,
                                            dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildAlterRecord( const dpsLogRecord &record,
                                      bson::BSONObjBuilder &builder,
                                      dpsRecordTransInfo &transInfo,
                                      dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildAddUniqueIDRecord( const dpsLogRecord &record,
                                            bson::BSONObjBuilder &builder,
                                            dpsRecordTransInfo &transInfo,
                                            dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildReturnRecord( const dpsLogRecord &record,
                                       bson::BSONObjBuilder &builder,
                                       dpsRecordTransInfo &transInfo,
                                       dpsRecordTimeInfo &timeInfo ) ;
      static INT32 _buildDefaultRecord( const dpsLogRecord &record,
                                        bson::BSONObjBuilder &builder,
                                        dpsRecordTransInfo &transInfo,
                                        dpsRecordTimeInfo &timeInfo ) ;
   } ;

   typedef class _rtnLogRecordSerializer rtnLogRecordSerializer ;

}

#endif // RTN_LOG_RECORD_SERIALIZER_HPP__
