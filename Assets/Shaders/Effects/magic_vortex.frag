// Magiewirbel: rotierende Spiralarme in Violett/Pink mit leuchtendem Kern.
uniform float progress; // 0..1 über die Effektdauer

void main()
{
	vec2 p = gl_TexCoord[0].xy * 2.0 - 1.0;
	p.y /= 0.7;

	float r = length(p);
	float a = atan(p.y, p.x);
	float env = sin(progress * 3.14159);

	float arms = pow(0.5 + 0.5 * sin(a * 3.0 - r * 14.0 + progress * 16.0), 3.0);
	float disc = 1.0 - smoothstep(0.55, 0.9, r);
	float hole = smoothstep(0.0, 0.2, r);
	float core = exp(-r * 7.0);

	float v = (arms * disc * (0.3 + 0.7 * hole) + core) * env;
	vec3 col = mix(vec3(0.75, 0.35, 1.0), vec3(1.0, 0.5, 0.85), clamp(r, 0.0, 1.0));
	col = mix(col, vec3(1.0), core * 0.6);
	gl_FragColor = vec4(col, clamp(v, 0.0, 1.0));
}
