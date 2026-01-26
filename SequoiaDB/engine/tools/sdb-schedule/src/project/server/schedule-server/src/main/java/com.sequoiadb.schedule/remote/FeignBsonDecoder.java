/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = FeignBsonDecoder.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.remote;

import com.fasterxml.jackson.databind.ObjectMapper;
import feign.FeignException;
import feign.Response;
import feign.codec.Decoder;
import feign.jackson.JacksonDecoder;

import java.io.IOException;
import java.lang.reflect.Type;
import java.util.Collections;
import java.util.Map;

public class FeignBsonDecoder implements Decoder {
    private JacksonDecoder jacksonDecoder = new JacksonDecoder();
    private Map<Type, Decoder> typeDecoders = Collections.emptyMap();

    public FeignBsonDecoder(ObjectMapper mapper, Map<Type, Decoder> typeDecoders) {
        if (mapper != null) {
            this.jacksonDecoder = new JacksonDecoder(mapper);
        }

        if (typeDecoders != null) {
            this.typeDecoders = typeDecoders;
        }
    }

    @Override
    public Object decode(Response response, Type type) throws IOException, FeignException {
        Decoder decoder = typeDecoders.get(type);
        if (decoder == null) {
            decoder = jacksonDecoder;
        }

        return decoder.decode(response, type);
    }
}
