uniform float time;
uniform vec2 resolution;

float hash(vec2 p)
{
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
	vec2 p = gl_TexCoord[0].xy * resolution;
	p.x += p.y * 0.18;
	vec2 grid = vec2(p.x / 16.0, (p.y - time * 440.0) / 80.0);
	vec2 cell = floor(grid);
	vec2 local = fract(grid);
	float offset = hash(cell);
	float streak = 1.0 - smoothstep(0.02, 0.10, abs(local.x - offset));
	float lengthMask = 1.0 - smoothstep(0.12, 0.34, abs(local.y - 0.5));
	float alpha = streak * lengthMask * (0.25 + 0.30 * hash(cell + 19.0));
	gl_FragColor = vec4(0.72, 0.82, 0.95, alpha);
}
