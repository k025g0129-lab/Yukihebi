#include "Sprite.hlsli"

#if !KAMATAENGINE_DYNAMIC_RESOURCES
Texture2D tex : register(t1);
#endif

SamplerState smp : register(s0);

float4 main(VSOutput input) : SV_TARGET { 
#if KAMATAENGINE_DYNAMIC_RESOURCES
    Texture2D tex = ResourceDescriptorHeap[NonUniformResourceIndex(input.textureDescriptorIndex)];
#endif
    return tex.Sample(smp, input.uv) * input.color;
}
