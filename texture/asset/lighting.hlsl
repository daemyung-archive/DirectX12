struct Input {
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;
};

struct Output {
    float4 sv_position : SV_POSITION;
    float3 position    : POSITION;
    float2 uv          : TEXCOORD0;
    float3 normal      : NORMAL;
};

cbuffer Constants : register(b0) {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float3x3 normal;
    float3 view_direction;
    float light_distance;
    float3 light_position;
    float light_spot_power;
    float3 light_color;
    float3 light_direction;
    int mip_slice;
};

Texture2D diffuse_texture : register(t0);
SamplerState linear_sampler : register(s0);

Output VSMain(Input input) {
    float4x4 PVM = mul(projection, mul(view, model));
    float4 position = float4(input.position, 1.0);

    Output output;
    output.sv_position = mul(PVM, position);
    output.position = mul(model, position).xyz;
    output.uv = input.uv;
    output.normal = mul(normal, input.normal);
    return output;
}

float4 GetTexel(float2 uv) {
    return diffuse_texture.SampleLevel(linear_sampler, uv, mip_slice);
}

float GetAttenuation(float distance) {
    return saturate((light_distance - distance) / light_distance);
}

float4 PSMain(Output input, bool is_front_face : SV_IsFrontFace) : SV_Target {
    float3 ambient = float3(0.1, 0.1, 0.1);
    float3 N = normalize(is_front_face ? input.normal : -input.normal);
    float3 L = input.position - light_position;

    float distance = length(L);
    if (distance > light_distance) {
        return float4(ambient, 1.0);
    }

    L /= distance;
    float spot_factor = dot(L, normalize(light_direction));
    if (spot_factor < 0.0) {
        return float4(ambient, 1.0);
    }

    spot_factor = pow(max(spot_factor, 0.0), light_spot_power);
    float4 texel = is_front_face ? GetTexel(input.uv) : GetTexel(input.uv).bgra;
    float3 diffuse = texel.rgb * max(dot(-L, N), 0.0);
    float3 V = normalize(view_direction);
    float3 R = reflect(-L, N);
    float3 specular = pow(max(dot(R, -V), 0.0), 128.0) * texel.a;
    float3 light = light_color * GetAttenuation(distance) * spot_factor;

    return float4(ambient + ((diffuse + specular) * light), 1.0);
}