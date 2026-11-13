// Gemeinsamer Vertex-Shader aller Magieeffekte: reicht Position und Quad-Koordinaten (0..1) durch.
void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
}
