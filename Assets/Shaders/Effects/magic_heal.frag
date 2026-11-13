// Heilung: pulsierender grüner Bodenring und aufsteigende, funkelnde Lichtpunkte.
uniform float progress; // 0..1 über die Effektdauer

float hash(float n)
{
	return fract(sin(n * 127.1) * 43758.5453);
}

void main()
{
	vec2 p = gl_TexCoord[0].xy * 2.0 - 1.0;
	float env = sin(progress * 3.14159);

	vec2 q = vec2(p.x, (p.y - 0.6) / 0.3);
	float ring = (1.0 - smoothstep(0.0, 0.05, abs(length(q) - (0.4 + 0.25 * progress)))) * env * 0.7;
	vec3 acc = vec3(0.4, 1.0, 0.5) * ring;

	for (int i = 0; i < 14; ++i)
	{
		float id = float(i);
		float t = clamp(progress * 1.25 - hash(id + 3.0) * 0.25, 0.0, 1.0);
		if (t > 0.0 && t < 1.0)
		{
			vec2 d = p - vec2((hash(id) - 0.5) * 1.1 + sin(t * 8.0 + id) * 0.06, 0.6 - t * 1.5);
			float star = exp(-length(d) * 30.0)
					   + (exp(-abs(d.x) * 60.0) * exp(-abs(d.y) * 10.0)
						+ exp(-abs(d.y) * 60.0) * exp(-abs(d.x) * 10.0)) * 0.6;
			acc += mix(vec3(0.4, 1.0, 0.5), vec3(1.0), 0.5) * star * sin(t * 3.14159);
		}
	}

	float a = clamp(max(acc.r, max(acc.g, acc.b)), 0.0, 1.0);
	gl_FragColor = vec4(acc / max(a, 0.001), a);
}
