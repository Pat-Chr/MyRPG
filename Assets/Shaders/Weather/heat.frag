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
	gl_FragColor = texture2D(scene, sampleUV) * gl_Color;
}
