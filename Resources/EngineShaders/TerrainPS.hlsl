#include "Terrain.hlsli"

#if !KAMATAENGINE_DYNAMIC_RESOURCES
Texture2D tex : register(t0);
#endif

SamplerState smp : register(s0);      // 0番スロットに設定されたサンプラー

static const float ambient = 0.3f;

float4 main(VSOutput input) : SV_TARGET {
	// テクスチャマッピング
#if KAMATAENGINE_DYNAMIC_RESOURCES
	Texture2D tex = ResourceDescriptorHeap[textureDescriptorIndex];
#endif
	float4 texcolor = tex.Sample(smp, input.uv);

	float3 light = normalize(float3(1, -1, 1));          // 右下奥　向きのライト
	float diffuse = saturate(dot(-light, input.normal)); // diffuseを[0,1]の範囲にClampする
	float brightness = diffuse + ambient;

	float4 shadecolor = float4(brightness, brightness, brightness, 1);

	// シェーディングによる色で描画
	return shadecolor * texcolor;
}
