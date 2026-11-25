// Arkane Druckwelle: heller Startblitz, zwei Ringe und Strahlen in Gold/Orange.
uniform float progress; // 0..1 across the effect duration

void main()
{
	vec2 p = gl_TexCoord[0].xy * 2.0 - 1.0;
	p.y /= 0.7;

	float r = length(p);
	float a = atan(p.y, p.x);
	float R = (1.0 - pow(1.0 - progress, 2.5)) * 0.95;
	float life = 1.0 - progress;

	float ring = exp(-abs(r - R) * 28.0) * life;
	float ring2 = exp(-abs(r - R * 0.7) * 40.0) * life * 0.6;
	float flash = exp(-r * 5.0) * exp(-progress * 7.0);
	float rays = pow(abs(cos(a * 5.0)), 30.0) * step(r, R) * exp(-r * 2.0) * life * 0.5;

	float v = clamp(ring + ring2 + flash + rays, 0.0, 1.0);
	vec3 col = mix(vec3(1.0, 0.55, 0.15), vec3(1.0, 0.95, 0.7), clamp(flash + ring, 0.0, 1.0));
	gl_FragColor = vec4(col, v);
}
