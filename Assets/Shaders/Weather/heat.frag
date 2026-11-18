uniform sampler2D scene;
uniform float time;
uniform vec2 resolution;
uniform vec2 worldOrigin;

void main()
{
	vec2 uv = gl_TexCoord[0].xy;
	vec2 pixel = vec2(uv.x, 1.0 - uv.y) * resolution + worldOrigin;

	float horizontal = sin(pixel.y * 0.035 + time * 2.0 + sin(pixel.x * 0.012 - time * 0.8) * 1.5);
	horizontal += 0.5 * sin(pixel.y * 0.075 + pixel.x * 0.018 + time * 3.1);
	float vertical = sin(pixel.x * 0.025 + time * 1.3 + sin(pixel.y * 0.02 + time) * 1.2);
	vertical += 0.4 * sin(pixel.x * 0.055 - pixel.y * 0.015 - time * 2.2);

	vec2 displacement = vec2(horizontal * 5.0, vertical * 2.0) / resolution;
	vec2 halfPixel = 0.5 / resolution;
	vec2 sampleUV = clamp(uv + displacement, halfPixel, vec2(1.0) - halfPixel);
	vec4 color = texture2D(scene, sampleUV);

	vec2 blurOffset = 8.0 / resolution;
	vec3 luminance = vec3(0.299, 0.587, 0.114);
	vec4 brightness;
	brightness.x = dot(texture2D(scene, clamp(sampleUV + vec2(blurOffset.x, 0.0), halfPixel, vec2(1.0) - halfPixel)).rgb, luminance);
	brightness.y = dot(texture2D(scene, clamp(sampleUV - vec2(blurOffset.x, 0.0), halfPixel, vec2(1.0) - halfPixel)).rgb, luminance);
	brightness.z = dot(texture2D(scene, clamp(sampleUV + vec2(0.0, blurOffset.y), halfPixel, vec2(1.0) - halfPixel)).rgb, luminance);
	brightness.w = dot(texture2D(scene, clamp(sampleUV - vec2(0.0, blurOffset.y), halfPixel, vec2(1.0) - halfPixel)).rgb, luminance);
	float glow = dot(smoothstep(vec4(0.45), vec4(0.9), brightness), vec4(0.25));
	color.rgb += (vec3(1.0) - color.rgb) * vec3(1.0, 0.52, 0.18) * (0.06 + glow * 0.18);
	gl_FragColor = color * gl_Color;
}
