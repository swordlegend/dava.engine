#include "common.slh"
#include "blending.slh"


////////////////////////////////////////////////////////////////////////////////
// fprog-input

fragment_in
{    
    #if MATERIAL_TEXTURE
        
        #if TEXTURE0_ANIMATION_SHIFT
        float2 varTexCoord0 : TEXCOORD0;
        #else
        float2 varTexCoord0 : TEXCOORD0;
        #endif

    #elif MATERIAL_SKYBOX
        
        float3 varTexCoord0 : TEXCOORD0;

    #endif

    #if MATERIAL_DECAL || ( MATERIAL_LIGHTMAP  && VIEW_DIFFUSE ) || FRAME_BLEND || ALPHA_MASK
        float2 varTexCoord1 : TEXCOORD1;
    #endif

    #if MATERIAL_DETAIL
        float2 varTexCoord1 : TEXCOORD1;
    #endif

    #if TILED_DECAL_MASK
        float2 varDecalTileTexCoord : TEXCOORD2;
    #endif
    #if MATERIAL_DETAIL
        half2 varDetailTexCoord : TEXCOORD2;
    #endif

    #if VERTEX_LIT

        half4 varDiffuseColor : COLOR0;

        #if BLINN_PHONG
            half4 varSpecularColor : TEXCOORD4;
        #elif NORMALIZED_BLINN_PHONG
            half4 varSpecularColor : TEXCOORD4;
        #endif

    #elif PIXEL_LIT
        
        #if FAST_NORMALIZATION
        half3 varHalfVec : COLOR0;
        #endif
        half3 varToLightVec : COLOR1;
        float3 varToCameraVec : TEXCOORD7;
    #endif
    
    #if VERTEX_COLOR || SPHERICAL_LIT
        half4 varVertexColor : COLOR1;
    #endif

    #if VERTEX_FOG
        half4 varFog : TEXCOORD5;
    #endif           

    #if FRAME_BLEND
        half varTime : TEXCOORD3;
    #endif

};



////////////////////////////////////////////////////////////////////////////////
// fprog-output

fragment_out
{
    float4 color : SV_TARGET0;
};





#if MATERIAL_TEXTURE
    uniform sampler2D albedo;
#elif MATERIAL_SKYBOX
    uniform samplerCUBE cubemap;
#endif

#if MATERIAL_DECAL
    uniform sampler2D decal;
#endif

#if ALPHA_MASK
    uniform sampler2D alphamask;
#endif

#if MATERIAL_DETAIL
    uniform sampler2D detail;
#endif

#if MATERIAL_LIGHTMAP  && VIEW_DIFFUSE
    uniform sampler2D lightmap;
#endif

#if MATERIAL_TEXTURE && ALPHATEST && ALPHATESTVALUE
    [statik][a] property float alphatestThreshold;
#endif

#if PIXEL_LIT
    uniform sampler2D normalmap;
    [statik][a] property float  inSpecularity               = 1.0;    
    [statik][a] property float3 metalFresnelReflectance     = float3(0.5,0.5,0.5);
    [statik][a] property float  normalScale                 = 1.0;
#endif


#if TILED_DECAL_MASK
    uniform sampler2D decalmask;
    uniform sampler2D decaltexture;
    [statik][a] property float4 decalTileColor = float4(1.0,1.0,1.0,1.0) ;
#endif


#if (VERTEX_LIT || PIXEL_LIT ) && (!SKYOBJECT)
    [dynamic][a] property float3 lightAmbientColor0;
    [dynamic][a] property float3 lightColor0;
    #if NORMALIZED_BLINN_PHONG && VIEW_SPECULAR
        [statik][a] property float inGlossiness = 0.5;
    #endif
#endif


#if PIXEL_LIT
    [dynamic][a] property float4 lightPosition0;
#endif


#if FLATCOLOR
    [statik][a] property float4 flatColor;
#endif

#if SETUP_LIGHTMAP && (MATERIAL_DECAL || MATERIAL_LIGHTMAP)
    [statik][a] property float lightmapSize;
#endif

float 
FresnelShlick( float NdotL, float Cspec )
{
    float expf = 5.0;
    return Cspec + (1.0 - Cspec) * pow(1.0 - NdotL, expf);
}

float3
FresnelShlickVec3( float NdotL, float3 Cspec )
{
    float expf = 5.0;
    return Cspec + (1.0 - Cspec) * (pow(1.0 - NdotL, expf));
}



////////////////////////////////////////////////////////////////////////////////
//

fragment_out
fp_main( fragment_in input )
{
    fragment_out    output;

    #if VERTEX_FOG
        float   varFogAmoung = input.varFog.a;
        float3  varFogColor  = input.varFog.rgb;
    #endif
    
    // FETCH PHASE
    #if MATERIAL_TEXTURE
    
        #if PIXEL_LIT || ALPHATEST || ALPHABLEND || VERTEX_LIT
            min10float4 textureColor0 = tex2D( albedo, input.varTexCoord0 );
            #if ALPHA_MASK 
                textureColor0.a *= FP_A8(tex2D( alphamask, input.varTexCoord1 ));
            #endif
        #else
            min10float3 textureColor0 = tex2D( albedo, input.varTexCoord0 ).rgb;
        #endif
            
        #if FRAME_BLEND
            min10float4 blendFrameColor = tex2D( albedo, input.varTexCoord1 );
            min10float varTime = input.varTime;
            textureColor0 = lerp( textureColor0, blendFrameColor, varTime );
        #endif
    
    #elif MATERIAL_SKYBOX
    
        min10float4 textureColor0 = texCUBE( cubemap, input.varTexCoord0 );
    
    #endif


    #if MATERIAL_TEXTURE
        #if ALPHATEST
            float alpha = textureColor0.a;
            #if VERTEX_COLOR
                alpha *= input.varVertexColor.a;
            #endif
            #if ALPHATESTVALUE
                if( alpha < alphatestThreshold ) discard;
            #else
                if( alpha < 0.5 ) discard;
            #endif
        #endif
    #endif

    
    #if MATERIAL_DECAL
        min10float3 textureColor1 = tex2D( decal, input.varTexCoord1 ).rgb;
    #endif
    
    
    #if MATERIAL_LIGHTMAP  && VIEW_DIFFUSE
        min10float3 textureColor1 = tex2D( lightmap, input.varTexCoord1 ).rgb;
    #endif
    
    
    #if MATERIAL_DETAIL
        min10float3 detailTextureColor = tex2D( detail, input.varDetailTexCoord ).rgb;
    #endif


    #if MATERIAL_DECAL || MATERIAL_LIGHTMAP
        #if SETUP_LIGHTMAP
            min10float3 lightGray = float3(0.75,0.75,0.75);
            min10float3 darkGray = float3(0.25,0.25,0.25);
    
            bool isXodd;
            bool isYodd;            
            
            if(fract(floor(input.varTexCoord1.x*lightmapSize)/2.0) == 0.0)
            {
                isXodd = true;
            }
            else
            {
                isXodd = false;
            }
            
            if(fract(floor(input.varTexCoord1.y*lightmapSize)/2.0) == 0.0)
            {
                isYodd = true;
            }
            else
            {
                isYodd = false;
            }
    
            if((isXodd && isYodd) || (!isXodd && !isYodd))
            {
                textureColor1 = lightGray;
            }
            else
            {
                textureColor1 = darkGray;
            }
        #endif
    #endif


    // DRAW PHASE

    #if VERTEX_LIT
    
        #if BLINN_PHONG
            
            min10float3 color = float3(0.0,0.0,0.0);
            #if VIEW_AMBIENT
                color += lightAmbientColor0;
            #endif

            #if VIEW_DIFFUSE
                color += float3(input.varDiffuseColor, input.varDiffuseColor, input.varDiffuseColor);
            #endif

            #if VIEW_ALBEDO
                #if TILED_DECAL_MASK
                    min10float maskSample = FP_A8(tex2D( decalmask, input.varTexCoord0 ));
                    min10float4 tileColor = tex2D( decaltexture, input.varDecalTileTexCoord ).rgba * decalTileColor;
                    color *= textureColor0.rgb + (tileColor.rgb - textureColor0.rgb) * tileColor.a * maskSample;
                #else
                    color *= textureColor0.rgb;
                #endif
            #endif

            #if VIEW_SPECULAR
                color += (input.varSpecularColor * textureColor0.a) * lightColor0;
            #endif
    
        #elif NORMALIZED_BLINN_PHONG
   
            min10float3 color = float3(0.0,0.0,0.0);
            
            #if VIEW_AMBIENT && !MATERIAL_LIGHTMAP
                color += lightAmbientColor0;
            #endif
        
            #if VIEW_DIFFUSE
                #if MATERIAL_LIGHTMAP
                    #if VIEW_ALBEDO
                        color = textureColor1.rgb * 2.0;
                    #else
                        //do not scale lightmap in view diffuse only case. artist request
                        color = textureColor1.rgb; 
                    #endif
                #else
                    color += input.varDiffuseColor * lightColor0;
                #endif
            #endif
        
            #if VIEW_ALBEDO
                #if TILED_DECAL_MASK
                    min10float maskSample = FP_A8(tex2D( decalmask, input.varTexCoord0 ));
                    min10float4 tileColor = tex2D( decaltexture, input.varDecalTileTexCoord ).rgba * decalTileColor;
                    color *= textureColor0.rgb + (tileColor.rgb - textureColor0.rgb) * tileColor.a * maskSample;
                #else
                    color *= textureColor0.rgb;
                #endif
            #endif
    
            #if VIEW_SPECULAR
                float glossiness = pow(5000.0, inGlossiness * textureColor0.a);
                float specularNorm = (glossiness + 2.0) / 8.0;
                float3 spec = input.varSpecularColor.xyz * pow(/*varNdotH*/FP_IN(varSpecularColor).w, glossiness) * specularNorm;
                                                     
                #if MATERIAL_LIGHTMAP
                    color += spec * textureColor1.rgb / 2.0 * lightColor0;
                #else
                    color += spec * lightColor0;
                #endif
            #endif
    
        #endif


    #elif PIXEL_LIT
        
        // lookup normal from normal map, move from [0, 1] to  [-1, 1] range, normalize
        float3 normal = 2.0 * tex2D( normalmap, input.varTexCoord0 ).rgb - 1.0;
        normal.xy *= normalScale;
        normal = normalize (normal);        
    
        #if !FAST_NORMALIZATION
            float3 toLightNormalized = normalize(input.varToLightVec.xyz);
            float3 toCameraNormalized = normalize(input.varToCameraVec);
            float3 H = toCameraNormalized + toLightNormalized;
            H = normalize(H);

            // compute diffuse lighting
            float NdotL = max (dot (normal, toLightNormalized), 0.0);
            float NdotH = max (dot (normal, H), 0.0);
            float LdotH = max (dot (toLightNormalized, H), 0.0);
            float NdotV = max (dot (normal, toCameraNormalized), 0.0);
        #else
            // Kwasi normalization :-)
            // compute diffuse lighting
            float3 normalizedHalf = normalize(input.varHalfVec.xyz);
            
            float NdotL = max (dot (normal, input.varToLightVec.xyz), 0.0);
            float NdotH = max (dot (normal, normalizedHalf), 0.0);
            float LdotH = max (dot (input.varToLightVec.xyz, normalizedHalf), 0.0);
            float NdotV = max (dot (normal, input.varToCameraVec), 0.0);
        #endif
    
        #if NORMALIZED_BLINN_PHONG
    
            #if DIELECTRIC
                #define ColorType float
                float fresnelOut = FresnelShlick( NdotV, dielectricFresnelReflectance );
            #else
                #if FAST_METAL
                    #define ColorType float
                    float fresnelOut = FresnelShlick( NdotV, (metalFresnelReflectance.r + metalFresnelReflectance.g + metalFresnelReflectance.b) / 3.0 );
                #else
                    #define ColorType float3
                    float3 fresnelOut = FresnelShlickVec3( NdotV, metalFresnelReflectance );
                #endif
            #endif
            
            
            //#define GOTANDA                        
            #if GOTANDA
                float3 fresnelIn = FresnelShlickVec3(NdotL, metalFresnelReflectance);
                float3 diffuse = NdotL / _PI * (1.0 - fresnelIn * inSpecularity);
            #else
                float diffuse = NdotL / _PI;// * (1.0 - fresnelIn * inSpecularity);
            #endif
        
            #if VIEW_SPECULAR
                //float glossiness = inGlossiness * 0.999;
                //glossiness = 200.0 * glossiness / (1.0 - glossiness);                
                float glossiness = inGlossiness * textureColor0.a;
                float glossPower = pow(5000.0, glossiness); //textureColor0.a;
                       
                #if GOTANDA
                    float specCutoff = 1.0 - NdotL;
                    specCutoff *= specCutoff;
                    specCutoff *= specCutoff;
                    specCutoff *= specCutoff;
                    specCutoff = 1.0 - specCutoff;
                #else
                    float specCutoff = NdotL;
                #endif
        
                #if GOTANDA
                    float specularNorm = (glossPower + 2.0) * (glossPower + 4.0) / (8.0 * _PI * (pow(2.0, -glossPower / 2.0) + glossPower));
                #else
                    float specularNorm = (glossPower + 2.0) / 8.0;
                #endif

                float specularNormalized = specularNorm * pow(NdotH, glossPower) * specCutoff * inSpecularity;

                #if FAST_METAL
                    float geometricFactor = 1.0;
                #else
                    float geometricFactor = 1.0 / LdotH * LdotH;
                #endif

                ColorType specular = specularNormalized * geometricFactor * fresnelOut;
            #endif
        
        #endif
    
        min10float3 color = min10float3(0.0,0.0,0.0);
    
        #if VIEW_AMBIENT && !MATERIAL_LIGHTMAP
            color += lightAmbientColor0;
        #endif
    
        #if VIEW_DIFFUSE
            #if MATERIAL_LIGHTMAP
                #if VIEW_ALBEDO
                    color = textureColor1.rgb * 2.0;
                #else
                    //do not scale lightmap in view diffuse only case. artist request
                    color = textureColor1.rgb; 
                #endif
            #else
                color += diffuse * lightColor0;
            #endif
        #endif
    
        #if VIEW_ALBEDO
            #if TILED_DECAL_MASK
                min10float maskSample = FP_A8( tex2D( decalmask, input.varTexCoord0 ));
                min10float4 tileColor = tex2D( decaltexture, input.varDecalTileTexCoord ).rgba * decalTileColor;
                color *= textureColor0.rgb + (tileColor.rgb - textureColor0.rgb) * tileColor.a * maskSample;
            #else
                color *= textureColor0.rgb;
            #endif
        #endif
    
        #if VIEW_SPECULAR
            #if MATERIAL_LIGHTMAP
                color += specular * textureColor1.rgb * lightColor0;
            #else
                color += specular * lightColor0;
            #endif
        #endif

    #else
        
        #if MATERIAL_DECAL || MATERIAL_LIGHTMAP
            
            float3 color = float3(0.0,0.0,0.0);

            #if VIEW_ALBEDO
                color = textureColor0.rgb;
            #else
                color = float3(1.0,1.0,1.0);
            #endif

            #if VIEW_DIFFUSE
                #if VIEW_ALBEDO
                    color *= textureColor1.rgb * 2.0;
                #else
                    //do not scale lightmap in view diffuse only case. artist request
                    color *= textureColor1.rgb; 
                #endif              
            #endif

        #elif MATERIAL_TEXTURE

            float3 color = textureColor0.rgb;
        
        #elif MATERIAL_SKYBOX
            
            float4 color = textureColor0;
        
        #else
            
            float3 color = float3(1.0,1.0,1.0);
        
        #endif
        
        #if TILED_DECAL_MASK
            min10float maskSample = FP_A8(tex2D( decalmask, input.varTexCoord0 ));
            min10float4 tileColor = tex2D( decaltexture, input.varDecalTileTexCoord ).rgba * decalTileColor;
            color.rgb += (tileColor.rgb - color.rgb) * tileColor.a * maskSample;
        #endif
        
    #endif


    #if MATERIAL_DETAIL
        color *= detailTextureColor.rgb * 2.0;
    #endif



    #if ALPHABLEND && MATERIAL_TEXTURE
        output.color = float4( color, textureColor0.a );
    #elif MATERIAL_SKYBOX
        output.color = color;
    #else
        output.color = float4( color, 1.0 );
    #endif

    
    #if VERTEX_COLOR || SPEED_TREE_LEAF || SPHERICAL_LIT
        output.color *= input.varVertexColor;
    #endif
        
    #if FLATCOLOR
        output.color *= flatColor;
    #endif




    
    
    #if VERTEX_FOG
        #if !FRAMEBUFFER_FETCH
            //VI: fog equation is inside of color equation for framebuffer fetch
            output.color.rgb = lerp( output.color.rgb, varFogColor, varFogAmoung );
        #endif
    #endif

    return output;
}

