uniform float time;
uniform vec2 resolution;

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
	vec2 p = gl_TexCoord[0].xy * resolution / 230.0;
	float clouds = 0.65 * noise(p + vec2(time * 0.05, 0.0));
	clouds += 0.35 * noise(p * 2.0 + vec2(-time * 0.08, time * 0.02));
	gl_FragColor = vec4(0.77, 0.83, 0.85, 0.15 + 0.4 * clouds);
}
