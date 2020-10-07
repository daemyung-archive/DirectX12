struct Input {
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct Output {
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

cbuffer Transformation : register(b0) {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};

Output VSMain(Input input) {
    float4x4 PVM = mul(mul(projection, view), model);

    Output output;
    output.position = mul(PVM, float4(input.position, 1.0));
    output.color = input.color;
    return output;
}

float4 PSMain(Output input) : SV_Target {
    float4 color;
    color = float4(input.color, 1.0);;
    return color;
}
