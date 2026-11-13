// Blitzeinschlag: drei flackernde Blitze von oben, Einschlagblitz und Druckring am Boden.
uniform float progress; // 0..1 über die Effektdauer

void main()
{
	vec2 p = gl_TexCoord[0].xy * 2.0 - 1.0;

	float fade = 1.0 - smoothstep(0.45, 1.0, progress);
	float flicker = 0.65 + 0.35 * sin(progress * 90.0);
	float impactY = 0.45;

	float glow = 0.0;
	for (int i = 0; i < 3; ++i)
	{
		// Form wird mehrmals pro Effekt neu gewürfelt, damit der Blitz zuckt
		float s = float(i) * 3.7 + floor(progress * 14.0) * 1.3;
		float off = (float(i) - 1.0) * 0.22;
		float toImpact = clamp((impactY - p.y) / (impactY + 1.0), 0.0, 1.0);
		float jag = (sin(p.y * 11.0 + s) + 0.5 * sin(p.y * 27.0 + s * 2.3)) * 0.06
					* clamp((impactY - p.y) * 2.0, 0.0, 1.0);
		float d = abs(p.x - off * toImpact - jag);
		float mask = step(p.y, impactY) * smoothstep(-1.0, -0.85, p.y);
		glow += (exp(-d * 45.0) * 0.8 + exp(-d * 180.0)) * mask;
	}

	vec2 q = p - vec2(0.0, impactY);
	q.y /= 0.45;
	float flash = exp(-length(q) * 5.0) * (1.0 - progress);
	float ring = (1.0 - smoothstep(0.0, 0.06, abs(length(q) - progress * 0.9))) * (1.0 - progress);

	float intensity = (glow * flicker + flash * 0.8) * fade + ring * 0.6;
	vec3 color = mix(vec3(0.5, 0.65, 1.0), vec3(1.0), clamp(glow, 0.0, 1.0));
	gl_FragColor = vec4(color, clamp(intensity, 0.0, 1.0));
}
