#include "Sprite.hlsli"

#if !KAMATAENGINE_DYNAMIC_RESOURCES
Texture2D tex : register(t1);
#endif

SamplerState smp : register(s0);

float3 ApplySRGBGamma(float3 linearColor)
{
    float3 low = 12.92 * linearColor;
    float3 high = 1.055 * pow(abs(linearColor), 1.0 / 2.4) - 0.055;
    return float3(linearColor.r < 0.0031308 ? low.r : high.r,
                  linearColor.g < 0.0031308 ? low.g : high.g,
                  linearColor.b < 0.0031308 ? low.b : high.b);
}

float4 main(VSOutput input) : SV_TARGET {
#if KAMATAENGINE_DYNAMIC_RESOURCES
    Texture2D tex = ResourceDescriptorHeap[NonUniformResourceIndex(input.textureDescriptorIndex)];
#endif
    float4 output = tex.Sample(smp, input.uv) * input.color;
    output.rgb = ApplySRGBGamma(output.rgb);
    return output;
}
