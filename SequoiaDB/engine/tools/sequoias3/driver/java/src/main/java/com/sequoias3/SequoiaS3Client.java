package com.sequoias3;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.SdkClientException;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.util.BinaryUtils;
import com.amazonaws.util.DateUtils;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;

import com.sequoias3.auth.SignerBase;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.core.InnerBucket;
import com.sequoias3.core.InnerGetRegionResponse;
import com.sequoias3.core.InnerRegion;
import com.sequoias3.exception.Error;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.ListRegionsResult;
import com.sequoias3.model.Region;
import com.sequoias3.util.RegionUtil;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.ResponseEntity;
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.client.*;
import com.sequoias3.auth.SignerForAuthorizationV4;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.*;

import static com.sequoias3.core.CommonDefine.*;

public class SequoiaS3Client implements SequoiaS3 {
    private static final Logger logger = LoggerFactory.getLogger(SequoiaS3Client.class);
    private static String contentHashStringNull = SignerBase.EMPTY_BODY_SHA256;

    private static String accessKeyId;
    private static String secretKeyId;
    private static String endpoint;
    private static RestTemplate rest;
    private HttpComponentsClientHttpRequestFactory factory;

    SequoiaS3Client(SequoiaS3ClientBuilder builder){
        this.accessKeyId = builder.getAccessKeyId();
        this.secretKeyId = builder.getSecretKeyId();
        this.endpoint = builder.getEndpoint();
        this.factory = new HttpComponentsClientHttpRequestFactory();
        this.factory.setConnectionRequestTimeout(builder.getConnectionRequestTimeout());
        this.factory.setConnectTimeout(builder.getConnectTimeout());
        this.factory.setBufferRequestBody(builder.isRequestBody());
        this.factory.setReadTimeout(builder.getReadTimeout());
        rest = new RestTemplate(this.factory);
    }

    @Override
    public void createRegion(String regionName) throws SdkClientException, AmazonServiceException {
        try {
            RegionUtil.isValidRegionName(regionName);

            // url = http://ip:port/region/?Action=CreateRegion&RegionName=regionName
            String url = endpoint + PATH_REGION + "?" +
                    ACTION + "=" + CREATE_REGION + "&" +
                    REGION_NAME + "=" + regionName;
            URL endpointUrl = new URL(url);
            SignerForAuthorizationV4 signer = new SignerForAuthorizationV4(endpointUrl,
                    HTTP_METHOD_POST, SERVICE_NAME, regionName);

            Map<String, String> headers = new HashMap<>();
            headers.put(X_AMZ_CONTENT_SHA256, contentHashStringNull);
            Map<String, String> parameters = new HashMap<>();
            parameters.put(ACTION, CREATE_REGION);
            parameters.put(REGION_NAME, regionName);
            String authorization = signer.computeSignature(headers, parameters,
                    contentHashStringNull, accessKeyId, secretKeyId);
            headers.put(AUTHORIZATION, authorization);

            exec(headers, null, url, HttpMethod.POST, String.class);
        } catch (HttpStatusCodeException e){
            throw httpToAmazon(e);
        } catch (RestClientException e){
            throw new SdkClientException(e.getMessage(), e);
        } catch (MalformedURLException e){
            throw new IllegalArgumentException(e.getMessage(), e);
        }
    }

    @Override
    public void createRegion(CreateRegionRequest request) throws SdkClientException, AmazonServiceException {
        try {
            RegionUtil.validateRegion(request);
            InnerRegion region = new InnerRegion(request.getRegion());

            // url = http://ip:port/region/?Action=CreateRegion&RegionName=regionName
            String url = endpoint + PATH_REGION + "?" +
                    ACTION + "=" + CREATE_REGION + "&" +
                    REGION_NAME + "=" + region.getName();
            URL endpointUrl = new URL(url);
            SignerForAuthorizationV4 signer = new SignerForAuthorizationV4(endpointUrl,
                    HTTP_METHOD_POST, SERVICE_NAME, region.getName());

            String contentHashString = BinaryUtils.toHex(SignerBase.hash(region.toString()));
            Map<String, String> headers = new HashMap<>();
            headers.put(X_AMZ_CONTENT_SHA256, contentHashString);
            Map<String, String> parameters = new HashMap<>();
            parameters.put(ACTION, CREATE_REGION);
            parameters.put(REGION_NAME, region.getName());
            String authorization = signer.computeSignature(headers, parameters,
                    contentHashString, accessKeyId, secretKeyId);
            headers.put(AUTHORIZATION, authorization);

            exec(headers, region, url, HttpMethod.POST, String.class);
        } catch (HttpStatusCodeException e){
            throw httpToAmazon(e);
        } catch (RestClientException e){
            throw new SdkClientException(e.getMessage(), e);
        } catch (MalformedURLException e){
            throw new IllegalArgumentException(e.getMessage(), e);
        }
    }

    @Override
    public void deleteRegion(String regionName) throws SdkClientException, AmazonServiceException {
        try {
            // url = http://ip:port/region/?Action=DeleteRegion&RegionName=regionName
            String url = endpoint + PATH_REGION + "?" +
                    ACTION + "=" + DELETE_REGION + "&" +
                    REGION_NAME+"="+regionName;
            URL endpointUrl = new URL(url);

            Map<String, String> headers = new HashMap<>();
            headers.put(X_AMZ_CONTENT_SHA256, contentHashStringNull);
            Map<String, String> parameters = new HashMap<>();
            parameters.put(ACTION, DELETE_REGION);
            parameters.put(REGION_NAME, regionName);
            SignerForAuthorizationV4 signer = new SignerForAuthorizationV4(endpointUrl,
                    HTTP_METHOD_POST, SERVICE_NAME, regionName);
            String authorization = signer.computeSignature(headers, parameters,
                    contentHashStringNull, accessKeyId, secretKeyId);

            headers.put(AUTHORIZATION, authorization);

            exec(headers, null, url, HttpMethod.POST, String.class);
        } catch (HttpStatusCodeException e){
            throw httpToAmazon(e);
        } catch (RestClientException e){
            throw new SdkClientException(e.getMessage(), e);
        } catch (MalformedURLException e){
            throw new IllegalArgumentException(e.getMessage(), e);
        }
    }

    @Override
    public GetRegionResult getRegion(String regionName) throws SdkClientException, AmazonServiceException {
        ResponseEntity<?> resp;
        GetRegionResult result;
        try {
            // url = http://ip:port/region/?Action=GetRegion&RegionName=regionName
            String url = endpoint + PATH_REGION + "?" +
                    ACTION + "=" + GET_REGION + "&" +
                    REGION_NAME + "=" + regionName;
            URL endpointUrl = new URL(url);
            SignerForAuthorizationV4 signer = new SignerForAuthorizationV4(endpointUrl,
                    HTTP_METHOD_POST, SERVICE_NAME, regionName);

            Map<String, String> headers = new HashMap<>();
            headers.put(X_AMZ_CONTENT_SHA256, contentHashStringNull);
            Map<String, String> parameters = new HashMap<>();
            parameters.put(ACTION, GET_REGION);
            parameters.put(REGION_NAME, regionName);
            String authorization = signer.computeSignature(headers, parameters,
                    contentHashStringNull, accessKeyId, secretKeyId);
            headers.put(AUTHORIZATION, authorization);

            ResponseEntity<InnerGetRegionResponse> response = exec(headers,
                    null, url, HttpMethod.POST, InnerGetRegionResponse.class);
            result = new GetRegionResult();
            result.setRegion(regionConvert(response.getBody()));

            List<Bucket> bucketList = new ArrayList<Bucket>();
            List<InnerBucket> innerBucket = response.getBody().getBuckets();

            if(innerBucket != null) {
                for (int i = 0; i < innerBucket.size(); i++) {
                    Bucket bucket = new Bucket();
                    bucket.setName(innerBucket.get(i).getBucketName());
                    bucket.setCreationDate(DateUtils.parseISO8601Date(innerBucket.get(i).getFormatDate()));
                    bucketList.add(bucket);
                }
            }
            result.setBuckets(bucketList);

            return result;
        } catch (HttpStatusCodeException e){
            throw httpToAmazon(e);
        } catch (RestClientException e){
            throw new SdkClientException(e.getMessage(), e);
        } catch (MalformedURLException e){
            throw new IllegalArgumentException(e.getMessage(), e);
        }
    }

    @Override
    public boolean headRegion(String regionName) throws SdkClientException, AmazonServiceException {
        try {
            // url = http://ip:port/region/?Action=HeadRegion&RegionName=regionName
            String url = endpoint + PATH_REGION + "?" +
                    ACTION + "=" + HEAD_REGION + "&" +
                    REGION_NAME + "=" + regionName;
            URL endpointUrl = new URL(url);
            SignerForAuthorizationV4 signer = new SignerForAuthorizationV4(endpointUrl,
                    HTTP_METHOD_POST, SERVICE_NAME, regionName);

            Map<String, String> headers = new HashMap<>();
            headers.put(X_AMZ_CONTENT_SHA256, contentHashStringNull);
            Map<String, String> parameters = new HashMap<>();
            parameters.put(ACTION, HEAD_REGION);
            parameters.put(REGION_NAME, regionName);
            String authorization = signer.computeSignature(headers, parameters,
                    contentHashStringNull, accessKeyId, secretKeyId);

            headers.put(AUTHORIZATION, authorization);

            exec(headers, null, url, HttpMethod.POST, String.class);
            return true;
        } catch (HttpStatusCodeException e) {
            if (e.getStatusCode().value() != 404) {
                throw httpToAmazon(e);
            }
            return false;
        } catch (RestClientException e){
            throw new SdkClientException(e.getMessage(), e);
        } catch (MalformedURLException e){
            throw new IllegalArgumentException(e.getMessage(), e);
        }
    }

    @Override
    public ListRegionsResult listRegions() throws SdkClientException, AmazonServiceException {
        ListRegionsResult listResult;
        try {
            // url = http://ip:port/region/?Action=ListRegions
            String url = endpoint + PATH_REGION + "?" + ACTION + "=" + LIST_REGION;
            URL endpointUrl = new URL(url);

            Map<String, String> headers = new HashMap<>();
            headers.put(X_AMZ_CONTENT_SHA256, contentHashStringNull);
            Map<String, String> parameters = new HashMap<>();
            parameters.put(ACTION, LIST_REGION);
            SignerForAuthorizationV4 signer = new SignerForAuthorizationV4(endpointUrl,
                    HTTP_METHOD_POST, SERVICE_NAME, "");
            String authorization = signer.computeSignature(headers, parameters,
                    contentHashStringNull, accessKeyId, secretKeyId);
            headers.put(AUTHORIZATION, authorization);

            ResponseEntity<ListRegionsResult> resp = exec(headers, null, url, HttpMethod.POST, ListRegionsResult.class);

            listResult = resp.getBody();
            return listResult;
        } catch (HttpStatusCodeException e){
            throw httpToAmazon(e);
        } catch (RestClientException e){
            throw new SdkClientException(e.getMessage(), e);
        } catch (MalformedURLException e){
            throw new IllegalArgumentException(e.getMessage(), e);
        }
    }

    @Override
    public void shutdown(){
        try {
            this.factory.destroy();
        }catch (Exception e){
            logger.error("shutdown failed.", e);
        }
    }

    private AmazonServiceException httpToAmazon(HttpStatusCodeException e) {
        AmazonServiceException amazonS3Exception = new AmazonS3Exception(e.getMessage());
        amazonS3Exception.setStatusCode(e.getStatusCode().value());

        try {
            ObjectMapper objectMapper = new XmlMapper();
            String errorMessage = e.getResponseBodyAsString();
            Error result = objectMapper.readValue(errorMessage, Error.class);
            amazonS3Exception.setErrorCode(result.getCode());
            amazonS3Exception.setErrorMessage(result.getMessage());
        }catch (Exception e1){
            amazonS3Exception.setErrorCode(e.getStatusCode().toString());
            amazonS3Exception.setErrorMessage(e.getResponseBodyAsString());
            logger.warn("Parse error message failed.", e);
        }
        return amazonS3Exception;
    }

    private <T> ResponseEntity<T> exec(Map headers, Object body, String url,
                                   HttpMethod method, Class<T> responseType){
        MultiValueMap httpHeaders = new LinkedMultiValueMap();
        Set<String> headerKeySet = headers.keySet();
        for (String key : headerKeySet){
            httpHeaders.set(key, headers.get(key));
        }

        HttpEntity<?> requestEntity;
        if (body != null){
            requestEntity = new HttpEntity<>(body, httpHeaders);
        }else {
            requestEntity = new HttpEntity<>(httpHeaders);
        }

        ResponseEntity<T> response = rest.exchange(url, method, requestEntity, responseType);
        return response;
    }


    Region regionConvert(InnerRegion innerregion) {
        Region region = new Region();
        region.setName(innerregion.getName());
        region.setDataCSShardingType(DataShardingType.getType(innerregion.getDataCSShardingType()));
        region.setDataCLShardingType(DataShardingType.getType(innerregion.getDataCLShardingType()));
        region.setDataCSRange(innerregion.getDataCSRange());
        region.setDataDomain(innerregion.getDataDomain());
        region.setDataLobPageSize(innerregion.getDataLobPageSize());
        region.setDataReplSize(innerregion.getDataReplSize());
        region.setMetaDomain(innerregion.getMetaDomain());
        region.setDataLocation(innerregion.getDataLocation());
        region.setMetaLocation(innerregion.getMetaLocation());
        region.setMetaHisLocation(innerregion.getMetaHisLocation());
        return  region;
    }

}
