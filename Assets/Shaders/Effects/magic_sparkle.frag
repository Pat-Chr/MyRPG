// Sparkle-Effekt: abgeflachter Ring plus 12 aufsteigende Funken, vollständig auf der GPU berechnet.
uniform float progress; // 0..1 über die Effektdauer

const float PI = 3.14159265;

void main()
{
	vec2 p = gl_TexCoord[0].xy * 2.0 - 1.0; // -1..1 innerhalb des Quads
	p.y /= 0.45;                            // Ring perspektivisch abflachen

	float fade = 1.0 - progress;

	float radius = mix(0.25, 0.75, progress);
	float ring = smoothstep(0.06, 0.0, abs(length(p) - radius));

	float sparks = 0.0;
	for (int i = 0; i < 12; ++i)
	{
		float a = 2.0 * PI * float(i) / 12.0 + progress * 2.0;
		vec2 c = vec2(cos(a), sin(a) * 0.4) * mix(0.25, 0.6, progress);
		c.y -= progress * 0.4;
		sparks += smoothstep(0.07, 0.0, length(p - c));
	}

	float intensity = clamp(ring + sparks, 0.0, 1.0) * fade;
	vec3 color = mix(vec3(0.47, 0.78, 1.0), vec3(0.75, 0.88, 1.0), clamp(sparks, 0.0, 1.0));
	gl_FragColor = vec4(color, intensity);
}
