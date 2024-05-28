#include "SceneList.hpp"
#include "Assets/Material.hpp"
#include "Assets/Model.hpp"
#include "Assets/Texture.hpp"
#include <functional>
#include <random>

using namespace glm;

using Assets::Material;
using Assets::Model;
using Assets::Texture;

namespace
{

	void AddRayTracingInOneWeekendCommonScene(std::vector<Assets::Node>& nodes, std::vector<Assets::Model>& models, const bool& isProc, std::function<float ()>& random)
	{
		for (int i = -11; i < 11; ++i)
		{
			for (int j = -11; j < 11; ++j)
			{
				const float chooseMat = random();
				const float center_y = static_cast<float>(j) + 0.9f * random();
				const float center_x = static_cast<float>(i) + 0.9f * random();
				const vec3 center(center_x, 0.2f, center_y);

				if (length(center - vec3(4, 0.2f, 0)) > 0.9f)
				{
					if (chooseMat < 0.8f) // Diffuse
					{
						const float b = random() * random();
						const float g = random() * random();
						const float r = random() * random();

						models.push_back(Model::CreateSphere(center, 0.2f, Material::Lambertian(vec3(r, g, b)), isProc));
						nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), static_cast<int>(models.size() - 1), isProc) );
					}
					else if (chooseMat < 0.95f) // Metal
					{
						const float fuzziness = 0.5f * random();
						const float b = 0.5f * (1 + random());
						const float g = 0.5f * (1 + random());
						const float r = 0.5f * (1 + random());

						models.push_back(Model::CreateSphere(center, 0.2f, Material::Metallic(vec3(r, g, b), fuzziness), isProc));
						nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), static_cast<int>(models.size() - 1), isProc) );
					}
					else // Glass
					{
						const float fuzziness = 0.5f * random();
						models.push_back(Model::CreateSphere(center, 0.2f, Material::Dielectric(1.45f, fuzziness), isProc));
						nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), static_cast<int>(models.size() - 1), isProc) );
					}
				}
			}
		}
	}
}

const std::vector<std::pair<std::string, std::function<void (SceneList::CameraInitialSate&, std::vector<Assets::Node>& nodes, std::vector<Assets::Model>&, std::vector<Assets::Texture>&)>>> SceneList::AllScenes =
{
	{"Cube And Spheres", CubeAndSpheres},
	{"Ray Tracing In One Weekend", RayTracingInOneWeekend},
	{"Cornell Box", CornellBox},
	{"Cornell Box & Lucy", CornellBoxLucy},
	{"LivingRoom", LivingRoom},
	{"Kitchen", Kitchen},
	{"LuxBall", LuxBall},
	{"Still", Still},
};

void SceneList::CubeAndSpheres(CameraInitialSate& camera, std::vector<Assets::Node>& nodes, std::vector<Assets::Model>& models, std::vector<Assets::Texture>& textures)
{
	camera.ModelView = lookAt(vec3(13, 2, 3), vec3(0, 0, 0), vec3(0, 1, 0));
	camera.FieldOfView = 20;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 1000.0f;
	camera.ControlSpeed = 5.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	const auto i = mat4(1);
	
	auto scene = Model::LoadModel("../assets/models/simple.glb", textures);
	models.push_back(scene);
}

void SceneList::RayTracingInOneWeekend(CameraInitialSate& camera, std::vector<Assets::Node>& nodes, std::vector<Assets::Model>& models, std::vector<Assets::Texture>& textures)
{
	// Final scene from Ray Tracing In One Weekend book.
	
	camera.ModelView = lookAt(vec3(13, 2, 3), vec3(0, 0, 0), vec3(0, 1, 0));
	camera.FieldOfView = 20;
	camera.Aperture = 0.1f;
	camera.FocusDistance = 1000.0f;
	camera.ControlSpeed = 5.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	const bool isProc = true;

	std::mt19937 engine(42);
	std::function<float ()> random = std::bind(std::uniform_real_distribution<float>(), engine);
	models.push_back(Model::CreateBox(vec3(-1000, -0.5, -1000), vec3(1000, 0, 1000), Material::Lambertian(vec3(0.4f, 0.4f, 0.4f))));
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), static_cast<int>(models.size() - 1), false) );
	
	AddRayTracingInOneWeekendCommonScene(nodes, models, isProc, random);

	models.push_back(Model::CreateSphere(vec3(0, 1, 0), 1.0f, Material::Dielectric(1.5f, 0.5f), isProc));
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), static_cast<int>(models.size() - 1), isProc) );
	models.push_back(Model::CreateSphere(vec3(-4, 1, 0), 1.0f, Material::Lambertian(vec3(0.4f, 0.2f, 0.1f)), isProc));
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), static_cast<int>(models.size() - 1), isProc) );
	models.push_back(Model::CreateSphere(vec3(4, 1, 0), 1.0f, Material::Metallic(vec3(0.7f, 0.6f, 0.5f), 0.3f), isProc));
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), static_cast<int>(models.size() - 1), isProc) );
}

void SceneList::CornellBox(CameraInitialSate& camera,std::vector<Assets::Node>& nodes, std::vector<Assets::Model>& models, std::vector<Assets::Texture>& textures)
{
	camera.ModelView = lookAt(vec3(0, 278, 1078), vec3(0, 278, 0), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 778.0f;
	camera.ControlSpeed = 200.0f;
	camera.GammaCorrection = true;
	camera.HasSky = false;

	const auto i = mat4(1);
	const auto white = Material::Lambertian(vec3(0.73f, 0.73f, 0.73f));

	auto box0 = Model::CreateBox(vec3(0, 0, -165), vec3(165, 165, 0), white);
	auto box1 = Model::CreateBox(vec3(0, 0, -165), vec3(165, 165, 0), white);

	glm::mat4 ts0 = glm::transpose(rotate(translate(i, vec3(278 - 130 - 165, 0, 213)), radians(-18.0f), vec3(0, 1, 0)));
	glm::mat4 ts1 = glm::transpose(rotate(scale(translate(i, vec3(278 - 265 - 165, 0, 17)), vec3(1,2,1)), radians(15.0f), vec3(0, 1, 0)));
	
	
	models.push_back(Model::CreateCornellBox(555));
	models.push_back(box0);

	nodes.push_back(  Assets::Node::CreateNode(mat4(1), 0, false) );
	nodes.push_back(  Assets::Node::CreateNode(ts0, 1, false) );
	nodes.push_back(  Assets::Node::CreateNode(ts1, 1, false) );
}

void SceneList::CornellBoxLucy(CameraInitialSate& camera, std::vector<Assets::Node>& nodes, std::vector<Assets::Model>& models, std::vector<Assets::Texture>& textures)
{
	camera.ModelView = lookAt(vec3(0, 278, 1078), vec3(0, 278, 0), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 100.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = false;
	
	const auto i = mat4(1);
	const auto sphere = Model::CreateSphere(vec3(278 - 130, 165.0f, -165.0f / 2 + 213), 80.0f, Material::Dielectric(1.45f, 0.0f), true);
	auto lucy0 = Model::LoadModel("../assets/models/lucy.obj", textures);
	
	glm::mat4 ts2 = glm::transpose(	rotate(
			scale(
				translate(i, vec3(278 - 300 - 165/2, -9, 17 - 165/2)),
				vec3(0.6f)),
			radians(75.0f), vec3(0, 1, 0)));
	

	models.push_back(Model::CreateCornellBox(555));
	models.push_back(sphere);
	models.push_back(lucy0);

	nodes.push_back( Assets::Node::CreateNode(glm::mat4(1), 0, false));
	nodes.push_back( Assets::Node::CreateNode(glm::mat4(1), 1, true));
	nodes.push_back( Assets::Node::CreateNode(ts2, 2, false));
}

void SceneList::LivingRoom(CameraInitialSate& camera, std::vector<Assets::Node>& nodes, std::vector<Assets::Model>& models, std::vector<Assets::Texture>& textures)
{
	camera.ModelView = lookAt(vec3(0, 1.5, 8), vec3(0, 1.5, 4.5), vec3(0, 1, 0));
	camera.FieldOfView = 45;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 100.0f;
	camera.ControlSpeed = 1.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;
	
	const auto arealight = Material::DiffuseLight(vec3(1000,1000,1000));
	auto light = Model::CreateQuad( vec3(-2, .8, -0.5), vec3(-2, 3, -0.5), vec3(2, 3, -0.5), vec3(2, .8, -0.5),
		vec3(0,0,1), arealight );
	auto objfile = Model::LoadModel("../assets/models/livingroom.obj", textures);
	
	models.push_back(light);
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), 0, false) );
	models.push_back(objfile);
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), 1, false) );
}

void SceneList::Kitchen(CameraInitialSate& camera, std::vector<Assets::Node>& nodes, std::vector<Assets::Model>& models, std::vector<Assets::Texture>& textures)
{
	camera.ModelView = lookAt(vec3(2, 1.5, 3), vec3(-4, 1.5, -4), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 100.0f;
	camera.ControlSpeed = 1.0f;
	camera.GammaCorrection = true;
	camera.HasSky = false;
	
	const auto arealight = Material::DiffuseLight(vec3(1000,1000,1000));
	auto light = Model::CreateQuad( vec3(-1, .8, -3.2), vec3(-1, 3, -3.2), vec3(1, 3, -3.2), vec3(1, .8, -3.2),
		vec3(0,0,1), arealight );
	
	auto objfile = Model::LoadModel("../assets/models/kitchen.obj", textures);
	models.push_back(light);
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), 0, false) );
	models.push_back(objfile);
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), 1, false) );
}

void SceneList::LuxBall(CameraInitialSate& camera, std::vector<Assets::Node>& nodes, std::vector<Assets::Model>& models, std::vector<Assets::Texture>& textures)
{
	camera.ModelView = lookAt(vec3(0.168, 0.375, 0.487), vec3(0, 0.05, 0.0), vec3(0, 1, 0));
	camera.FieldOfView = 20;
	camera.Aperture = 0.00f;
	camera.FocusDistance = 55.0f;
	camera.ControlSpeed = 0.1f;
	camera.GammaCorrection = true;
	camera.HasSky = false;
	
	auto objfile = Model::LoadModel("../assets/models/luxball.obj", textures);
	models.push_back(objfile);
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), 0, false) );
}

void SceneList::Still(CameraInitialSate& camera, std::vector<Assets::Node>& nodes, std::vector<Assets::Model>& models, std::vector<Assets::Texture>& textures)
{
	camera.ModelView = lookAt(vec3(0.031, 0.26, 2.454), vec3(0.031, 0.26, 2), vec3(0, 1, 0));
	camera.FieldOfView = 16;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 100.0f;
	camera.ControlSpeed = 0.2f;
	camera.GammaCorrection = true;
	camera.HasSky = false;
	
	auto objfile = Model::LoadModel("../assets/models/still1.obj", textures);
	models.push_back(objfile);
	nodes.push_back( Assets::Node::CreateNode( glm::mat4(1), 0, false) );
}
