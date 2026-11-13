// Feuerstoß: aufsteigende Flammenpartikel mit Farbverlauf Gelb -> Orange -> Rot.
uniform float progress; // 0..1 über die Effektdauer

float hash(float n)
{
	return fract(sin(n * 127.1) * 43758.5453);
}

void main()
{
	vec2 p = gl_TexCoord[0].xy * 2.0 - 1.0;
	vec3 acc = vec3(0.0);

	for (int i = 0; i < 18; ++i)
	{
		float id = float(i);
		float t = clamp(progress * 1.4 - hash(id) * 0.4, 0.0, 1.0);
		if (t > 0.0 && t < 1.0)
		{
			vec2 pos = vec2((hash(id + 7.0) - 0.5) * 0.8 * (1.0 - t * 0.6) + sin(t * 6.0 + id) * 0.08,
							0.6 - t * 1.4);
			float size = mix(0.2, 0.04, t);
			float g = (1.0 - smoothstep(0.0, size, length(p - pos))) * (1.0 - t);
			vec3 col = mix(vec3(1.0, 0.9, 0.35), vec3(0.9, 0.2, 0.05), t);
			acc += col * g;
		}
	}

	float baseGlow = exp(-length(vec2(p.x, (p.y - 0.6) / 0.4)) * 3.0) * sin(progress * 3.14159) * 0.5;
	acc += vec3(1.0, 0.4, 0.1) * baseGlow;

	float a = clamp(max(acc.r, max(acc.g, acc.b)), 0.0, 1.0);
	gl_FragColor = vec4(acc / max(a, 0.001), a);
}
