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

   Source File Name = hitTransferHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_HIT_TRANSFER_HANDLER_H_
#define VESSEL_HIT_TRANSFER_HANDLER_H_

#include "vessel/requestHandler.h"

namespace engine
{
namespace vessel
{
   class hitTransferTaskCtx;

   class hitTransferHandler : public requestHandler
   {
      public:
         hitTransferHandler() = default;
         virtual ~hitTransferHandler() = default;

      public:
         INT32 handle(hitTransferTaskCtx *ctx);

      private:
         hitTransferTaskCtx *_ctx = nullptr;
   };//class hitTransferHandler
} // namespace vessel

} // namespace engine


#endif//VESSEL_HIT_TRANSFER_HANDLER_H_