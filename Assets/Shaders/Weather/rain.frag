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

void main()
{
	vec2 screenPixel = gl_TexCoord[0].xy * resolution;
	vec2 cloudPosition = (screenPixel + worldOrigin - vec2(18.0, 6.0) * time) / 280.0;
	float clouds = 0.7 * noise(cloudPosition) + 0.3 * noise(cloudPosition * 2.1);
	float shadowAlpha = 0.38 * smoothstep(0.28, 0.72, clouds);

	vec2 p = screenPixel;
	p.x += p.y * 0.18;
	vec2 grid = vec2(p.x / 16.0, (p.y - time * 440.0) / 80.0);
	vec2 cell = floor(grid);
	vec2 local = fract(grid);
	float offset = hash(cell);
	float streak = 1.0 - smoothstep(0.02, 0.10, abs(local.x - offset));
	float lengthMask = 1.0 - smoothstep(0.12, 0.34, abs(local.y - 0.5));
	float rainAlpha = streak * lengthMask * (0.25 + 0.30 * hash(cell + 19.0));
	float alpha = rainAlpha + shadowAlpha * (1.0 - rainAlpha);
	vec3 color = vec3(0.72, 0.82, 0.95) * rainAlpha / max(alpha, 0.0001);
	gl_FragColor = vec4(color, alpha);
}
