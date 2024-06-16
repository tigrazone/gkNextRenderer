
struct UniformBufferObject
{
	mat4 ModelView;
	mat4 Projection;
	mat4 ModelViewInverse;
	mat4 ProjectionInverse;
	mat4 ViewProjection;
	mat4 PrevViewProjection;
	float Aperture;
	float FocusDistance;
	float SkyRotation;
	float HeatmapScale;
	float ColorPhi;
	float DepthPhi;
	float NormalPhi;
	float PaperWhiteNit;
	vec4 SunDirection;
	vec4 SunColor;
	uint TotalFrames;
	uint TotalNumberOfSamples;
	uint NumberOfSamples;
	uint NumberOfBounces;
	uint RandomSeed;
	uint LightCount;
	bool HasSky;
	bool ShowHeatmap;
	bool UseCheckerBoard;
	uint TemporalFrames;
	bool HasSun;
};
