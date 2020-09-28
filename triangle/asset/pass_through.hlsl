struct Input {
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct Output {
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

Output VSMain(Input input) {
    Output output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}

float4 PSMain(Output input) : SV_Target {
    float4 color;
    color = float4(input.color, 1.0);;
    return color;
}
