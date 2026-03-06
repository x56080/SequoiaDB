package com.sequoiadb.sdbschedule.utils;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;

public class BsonUtils {
    private static final ObjectMapper objectMapper = new ObjectMapper();

    /**
     * 将 JSON 字符串转为 BSONObject
     */
    public static BSONObject toBson( String json ) {
        try {
            if ( json == null ) {
                return null;
            }
            if ( json.isEmpty() ) {
                return new BasicBSONObject();
            }
            return ( BSONObject ) JSON.parse( json );
        } catch ( Exception e ) {
            throw new RuntimeException( "Failed to convert JSON to BSONObject",
                    e );
        }
    }

    /**
     * 直接把 RestAssured Response 转为 BSONObject
     */
    public static BSONObject fromResponse(
            io.restassured.response.Response resp ) {
        return toBson( resp.asString() );
    }
}
