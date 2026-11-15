uniform float progress;
uniform vec2 resolution;

void main()
{
	vec2 uv = gl_TexCoord[0].xy;
	vec2 centered = (uv - vec2(0.5)) * vec2(resolution.x / resolution.y, 1.0);
	float radius = length(centered);
	float pulse = sin(progress * 3.14159265);
	float ring = 1.0 - smoothstep(0.03, 0.12, abs(radius - progress * 0.8));
	float intensity = pulse * (0.18 + 0.35 * ring);
	gl_FragColor = vec4(0.1, 0.5, 1.0, intensity);
}
