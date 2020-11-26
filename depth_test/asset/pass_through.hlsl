struct Input {
    float3 position : POSITION;
    float3 normal   : NORMAL;
};

struct Output {
    float4 sv_position : SV_POSITION;
    float3 normal   : NORMAL;
};

cbuffer Transformations : register(b0) {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float3x3 normal;
};

Output VSMain(Input input) {
    float4x4 PVM = mul(mul(projection, view), model);

    Output output;
    output.sv_position = mul(PVM, float4(input.position, 1.0));
    output.normal = mul(normal, input.normal);

    return output;
}

float4 PSMain(Output input) : SV_Target {
    return float4(input.normal, 1.0);
}
