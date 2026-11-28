uniform float time;
uniform vec2 resolution;
uniform vec2 worldOrigin;

float hash(vec2 p)
{
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p)
{
	vec2 cell = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(cell), hash(cell + vec2(1.0, 0.0)), f.x),
			   mix(hash(cell + vec2(0.0, 1.0)), hash(cell + vec2(1.0, 1.0)), f.x), f.y);
}

float snowLayer(vec2 worldPixel, float cellSize, float speed, float seed)
{
	vec2 p = worldPixel;
	p.x += sin(p.y * 0.018 + time * 0.8 + seed) * 7.0 - time * 8.0;
	p.y -= time * speed;
	vec2 grid = p / cellSize;
	vec2 cell = floor(grid);
	vec2 center = vec2(0.25) + 0.5 * vec2(hash(cell + seed), hash(cell + seed + 37.0));
	float radius = mix(1.3, 3.0, hash(cell + seed + 71.0));
	float distanceToFlake = length((fract(grid) - center) * cellSize);
	return (1.0 - smoothstep(radius - 0.7, radius + 0.7, distanceToFlake)) *
		mix(0.55, 0.9, hash(cell + seed + 113.0));
}

void main()
{
	vec2 worldPixel = gl_TexCoord[0].xy * resolution + worldOrigin;
	vec2 cloudPosition = (worldPixel - vec2(18.0, 6.0) * time) / 280.0;
	float clouds = 0.7 * noise(cloudPosition) + 0.3 * noise(cloudPosition * 2.1);
	float shadowAlpha = 0.38 * smoothstep(0.28, 0.72, clouds);

	float nearSnow = snowLayer(worldPixel, 38.0, 62.0, 3.0);
	float farSnow = snowLayer(worldPixel, 54.0, 38.0, 29.0);
	float snowAlpha = nearSnow + farSnow * (1.0 - nearSnow);
	float alpha = snowAlpha + shadowAlpha * (1.0 - snowAlpha);
	vec3 color = vec3(0.92, 0.96, 1.0) * snowAlpha / max(alpha, 0.0001);
	gl_FragColor = vec4(color, alpha);
}
