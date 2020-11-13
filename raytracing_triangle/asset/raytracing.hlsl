RWTexture2D<float4> output : register(u0);
RaytracingAccelerationStructure scene : register(t0);

struct Vertex {
    float3 position;
    float3 color;
};

StructuredBuffer<Vertex> vertices : register(t1);

struct Payload {
    float4 color;
};

// all inverse matrices
cbuffer Transforms : register(b0) {
    float4x4 P;
    float4x4 V;
    float4x4 M;
}

[shader("raygeneration")]
void RayGeneration() {
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd/dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;

    RayDesc ray;
    ray.Origin = float3(0, 0, -2);
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));

    float4 cam_pos = float4(0, 0, 0, 1);
    cam_pos = mul(V, cam_pos);

    float4 target = float4(d.x, -d.y, 1, 1);
    target = mul(P, target);
    target = mul(V, target);

    ray.Origin = cam_pos.xyz;
    ray.Direction = target.xyz;

    ray.TMin = 0;
    ray.TMax = 100000;

    Payload payload;
    TraceRay( scene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, payload );

    output[launchIndex.xy] = payload.color;
}

[shader("miss")]
void Miss(inout Payload payload) {
    payload.color = float4(0.0, 0.0, 0.2, 1.0);
}

[shader("closesthit")]
void ClosestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attributes) {
    float3 barycentrics = float3(1.0 - attributes.barycentrics.x - attributes.barycentrics.y, attributes.barycentrics.x, attributes.barycentrics.y);

    uint id = 3 * PrimitiveIndex();
    const float3 A = vertices[id+0].color;
    const float3 B = vertices[id+1].color;
    const float3 C = vertices[id+2].color;

    payload.color = float4(A * barycentrics + B * barycentrics + C * barycentrics, 1.0);
}