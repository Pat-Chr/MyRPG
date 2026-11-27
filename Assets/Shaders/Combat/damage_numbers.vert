void main()
{
	// Vertex alpha carries remaining lifetime, allowing all numbers to share one draw call.
	float progress = 1.0 - gl_Color.a;
	float pop = sin(3.14159265 * min(progress / 0.25, 1.0));
	vec4 position = gl_Vertex;
	position.x += 6.0 * sin(progress * 3.14159265);
	position.y -= 36.0 * progress + 8.0 * pop;
	gl_Position = gl_ModelViewProjectionMatrix * position;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	float opacity = smoothstep(0.0, 0.06, progress) * (1.0 - smoothstep(0.6, 1.0, progress));
	gl_FrontColor = vec4(1.0, 1.0, 1.0, opacity);
}
