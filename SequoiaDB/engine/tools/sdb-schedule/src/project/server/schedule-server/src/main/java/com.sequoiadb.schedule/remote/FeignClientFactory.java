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

   Source File Name = FeignClientFactory.java

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

import com.fasterxml.jackson.databind.MapperFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.module.SimpleModule;
import feign.Feign;
import feign.InvocationHandlerFactory;
import feign.Logger;
import feign.Request;
import feign.Retryer;
import feign.codec.Decoder;
import feign.codec.Encoder;
import feign.codec.ErrorDecoder;
import feign.codec.StringDecoder;
import feign.form.spring.SpringFormEncoder;
import feign.httpclient.ApacheHttpClient;
import feign.slf4j.Slf4jLogger;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.springframework.beans.factory.ObjectFactory;
import org.springframework.boot.autoconfigure.http.HttpMessageConverters;
import org.springframework.cloud.openfeign.support.SpringEncoder;
import org.springframework.cloud.openfeign.support.SpringMvcContract;
import org.springframework.http.converter.json.MappingJackson2HttpMessageConverter;
import org.springframework.stereotype.Component;

import java.lang.reflect.Method;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;

/**
 * 动态 FeignClient 工厂，支持： - Apache HttpClient - BSON 编解码 - 自定义 ErrorDecoder - 全局
 * RequestInterceptor - 动态 host:port
 */
@Component
public class FeignClientFactory {

    private final Map<String, Object> cache = new ConcurrentHashMap<>();

    private final List<RequestInterceptor> globalRequestInterceptors = new ArrayList<>();

    private final Encoder defaultEncoder;
    private final Decoder defaultDecoder;
    private final ErrorDecoder defaultErrorDecoder = new CustomErrorDecoder();

    private final CloseableHttpClient httpClient = HttpClients.custom().build();

    // BSON 支持模块
    private static final SimpleModule bsonModule = new SimpleModule();

    Request.Options feignOptions =
            new Request.Options(10000, TimeUnit.MILLISECONDS,
                    180000, TimeUnit.MILLISECONDS,
                    true);

    static {
        bsonModule.addDeserializer(BSONObject.class, new BSONObjectJsonDeserializer<>());
        bsonModule.addDeserializer(BasicBSONList.class, new BSONObjectJsonDeserializer<>());
    }

    public FeignClientFactory() {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.configure(MapperFeature.IGNORE_DUPLICATE_MODULE_REGISTRATIONS, true);
        objectMapper.registerModule(bsonModule);

        Map<Type, Decoder> typeDecoders = new HashMap<>();
        typeDecoders.put(String.class, new StringDecoder());
        this.defaultDecoder = new FeignBsonDecoder(objectMapper, typeDecoders);

        // ✅ 支持 JSON 和表单混合编码
        ObjectFactory<HttpMessageConverters> messageConverters = () ->
                new HttpMessageConverters(new MappingJackson2HttpMessageConverter(objectMapper));

        this.defaultEncoder = new SpringFormEncoder(new SpringEncoder(messageConverters));

    }

    public void addRequestInterceptor(RequestInterceptor interceptor) {
        globalRequestInterceptors.add(interceptor);
    }

    @SuppressWarnings("unchecked")
    public <T> T getClient(Class<T> apiType, String hostPort) {
        String key = apiType.getName() + "@" + hostPort;
        return (T) cache.computeIfAbsent(key, k -> createClient(apiType, hostPort));
    }

    public <T> void removeClient(Class<T> apiType, String hostPort) {
        cache.remove(apiType.getName() + "@" + hostPort);
    }

    private <T> T createClient(Class<T> apiType, String hostPort) {
        Feign.Builder builder = Feign.builder().contract(new SpringMvcContract())
                .client(new ApacheHttpClient(httpClient)).encoder(defaultEncoder)
                .decoder(defaultDecoder).errorDecoder(defaultErrorDecoder)
                .logger(new Slf4jLogger(apiType)).logLevel(Logger.Level.BASIC)
                .options(feignOptions)
                .retryer(Retryer.NEVER_RETRY)
                .invocationHandlerFactory((target, dispatch) -> (proxy, method, args) -> {
                    for (RequestInterceptor interceptor : globalRequestInterceptors) {
                        interceptor.apply(proxy, method, args);
                    }
                    InvocationHandlerFactory.MethodHandler handler = dispatch.get(method);
                    if (handler != null) {
                        return handler.invoke(args);
                    }
                    return null;
                });

        return builder.target(apiType, "http://" + hostPort);
    }

    /**
     * 自定义请求拦截器接口
     */
    public interface RequestInterceptor {
        void apply(Object proxy, Method method, Object[] args);
    }

}
