// Frostnova: sich ausbreitender Eisring mit sechs Kristallspitzen, Strahlen und Glitzern.
uniform float progress; // 0..1 über die Effektdauer

void main()
{
	vec2 p = gl_TexCoord[0].xy * 2.0 - 1.0;
	p.y /= 0.6;

	float r = length(p);
	float a = atan(p.y, p.x);
	float ease = 1.0 - pow(1.0 - progress, 3.0);
	float R = ease * 0.85;
	float inside = 1.0 - smoothstep(0.0, R + 0.001, r);

	float spikes = pow(abs(cos(a * 3.0)), 10.0);
	float edge = 1.0 - smoothstep(0.0, 0.05, abs(r - (R + spikes * 0.15 * ease)));
	float fill = inside * 0.22;
	float rays = pow(abs(cos(a * 3.0)), 24.0) * inside * 0.7;
	float glint = pow(max(0.0, sin(a * 17.0 + r * 30.0)), 20.0) * inside * 0.35;

	float fade = 1.0 - smoothstep(0.5, 1.0, progress);
	float v = clamp(edge + fill + rays + glint, 0.0, 1.0) * fade;
	vec3 col = mix(vec3(0.35, 0.75, 1.0), vec3(0.9, 0.98, 1.0), clamp(edge + rays, 0.0, 1.0));
	gl_FragColor = vec4(col, v);
}
