struct Input {
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
};

struct Output {
    float4 sv_position : SV_POSITION;
    float2 uv          : TEXCOORD0;
};

cbuffer Constants : register(b0) {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4x4 uv_transform;
};

Texture2D diffuse_texture : register(t0);
SamplerState diffuse_sampler : register(s0);

Output VSMain(Input input) {
    float4x4 PVM = mul(projection, mul(view, model));

    Output output;
    output.sv_position = mul(PVM, float4(input.position,1.0));
    output.uv = mul(uv_transform, float4(input.uv, 0.0, 1.0)).xy;
    return output;
}

float4 PSMain(Output input) : SV_Target {
    return diffuse_texture.Sample(diffuse_sampler, input.uv);
}