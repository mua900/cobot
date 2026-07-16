
float2 worldToScreen(float2 p, float4 camera, float2 offset)
{
    float2 q = p;

	float s, c;
	sincos(camera.w, s, c);
    p = p - camera.xy;

    q.x = c * p.x - s * p.y;
    q.y = s * p.x + c * p.y;

	q = q * camera.z;

    return float2(q.x, -q.y) + offset;
}

float2 screenToWorld(float2 p, float4 camera, float2 offset)
{
	float s, c;
	sincos(-camera.w, s, c);

	p -= offset;

	p = float2(p.x, -p.y);
	p /= camera.z;

	float2 q;

    q.x = c * p.x - s * p.y;
    q.y = s * p.x + c * p.y;

    q = q + camera.xy;

    return q;
}
