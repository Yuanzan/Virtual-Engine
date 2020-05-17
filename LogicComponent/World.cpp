#include "World.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/Mesh.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../RenderComponent/GRPRenderManager.h"
#include "Transform.h"
#include "../CJsonObject/CJsonObject.hpp"
#include "../Common/Input.h"
#include "../Singleton/MeshLayout.h"
#include "../RenderComponent/Texture.h"
#include "../RenderComponent/MeshRenderer.h"
#include "../RenderComponent/PBRMaterial.h"

#include "../RenderComponent/Shader.h"
#include "../JobSystem/JobInclude.h"
#include "CameraMove.h"

#include "../ResourceManagement/AssetDatabase.h"
#include "../ResourceManagement/AssetReference.h"

#include "../RenderComponent/Light.h"
#include "../Common/GameTimer.h"
#include "../ResourceManagement/Scene.h"
#include "../LogicComponent/DirectionalLight.h"
#include "../RenderComponent/Skybox.h"
#include "../RenderComponent/ReflectionProbe.h"
using namespace Math;
using namespace neb;
World* World::current = nullptr;
#define MAXIMUM_RENDER_OBJECT 100000			//Take about 90M gpu memory

namespace WorldTester
{
	//	ObjectPtr<Transform> testObj;
	//	ObjectPtr<ITexture> testTex;
	ObjectPtr<Camera> mainCamera;
	StackObject<CameraMove> camMove;
	ObjectPtr<Scene> testScene;
	ObjectPtr<Transform> testLightTransform;
	ObjectPtr<Transform> sunTrans;
	DirectionalLight* sun;
}
using namespace WorldTester;
#include "../RenderComponent/RenderTexture.h"
World::World(ID3D12GraphicsCommandList* commandList, ID3D12Device* device) :
	allTransformsPtr(500)
{
	allCameras.reserve(10);
	current = this;
	mainCamera = ObjectPtr< Camera>::MakePtr(new Camera(device, Camera::CameraRenderPath::DefaultPipeline));
	allCameras.push_back(mainCamera);
	camMove.New(mainCamera);
	//testObj = Transform::GetTransform();
	//testObj->SetRotation(Vector4(-0.113517, 0.5999342, 0.6212156, 0.4912069));

	grpMaterialManager = new PBRMaterialManager(device, 256);
	//meshRenderer = new MeshRenderer(trans.operator->(), device, mesh, ShaderCompiler::GetShader("OpaqueStandard"));
	testLightTransform = Transform::GetTransform();
	testLight = Light::CreateLight(testLightTransform);
	testLight->SetLightType(Light::LightType_Spot);
	testLight->SetRange(10);
	testLight->SetIntensity(250);
	testLight->SetShadowNearPlane(0.2f);
	testLight->SetSpotAngle(90);
	testLight->SetSpotSmallAngle(10);
	testLight->SetEnabled(true);
	testLight->SetShadowEnabled(true, device);
	{
		Storage<CJsonObject, 1> jsonObjStorage;
		CJsonObject* jsonObj = (CJsonObject*)&jsonObjStorage;
		/*	if (ReadJson("Resource/Test.mat", jsonObj))
			{
				auto func = [=]()->void
				{
					jsonObj->~CJsonObject();
				};
				DestructGuard<decltype(func)> fff(func);
				pbrMat = ObjectPtr<PBRMaterial>::MakePtr(new PBRMaterial(device, grpMaterialManager, *jsonObj));
			}
			else
			{
				pbrMat = ObjectPtr<PBRMaterial>::MakePtr(new PBRMaterial(device, grpMaterialManager, CJsonObject("")));
			}*/
	}
	grpRenderer = new GRPRenderManager(
		//mesh->GetLayoutIndex(),
		MeshLayout::GetMeshLayoutIndex(true, true, false, true, true, false, false, false),
		MAXIMUM_RENDER_OBJECT,
		ShaderCompiler::GetShader("OpaqueStandard"),
		device
	);
	DXGI_FORMAT vtFormat[2] =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM
	};
	std::string res = "Resource";
	Runnable<void(const ObjectPtr<MObject>&)> sceneFunc = [&](const ObjectPtr<MObject>& value)->void
	{
		testScene = value.CastTo<Scene>();
	};
	AssetReference ref(AssetLoadType::Scene, "Scene_SampleScene", sceneFunc, true);
	AssetDatabase::GetInstance()->AsyncLoad(ref);
	sunTrans = Transform::GetTransform();
	sunTrans->SetRotation({ -0.0004715632f,0.9983955f,0.008083393f,0.05604406f });
	uint4 shadowRes = { 2048,2048,2048,2048 };
	sun = DirectionalLight::GetInstance(sunTrans, (uint*)&shadowRes, device);
	sun->intensity = 2;
	for (uint i = 0; i < 4; ++i)
	{
		sun->shadowSoftValue[i] *= 2;
	}
	ObjectPtr<Texture> skyboxTex = ObjectPtr< Texture>::MakePtr(new Texture(
		device,
		"Resource/Sky.vtex",
		TextureDimension::Cubemap
	));
	ObjectPtr<ITexture> it = skyboxTex.CastTo<ITexture>();
	currentSkybox = ObjectPtr<Skybox>::MakePtr(new Skybox(
		it,
		device
	));
}



World::~World()
{
	for (uint i = 0; i < allTransformsPtr.Length(); ++i)
	{
		Transform::DisposeTransform(allTransformsPtr[i]);
	}
	//	pbrMat.Destroy();
		//testTex.Destroy();
	camMove.Delete();
	mainCamera.Destroy();
	currentSkybox.Destroy();
	delete grpRenderer;
	delete grpMaterialManager;
}

bool vt_Initialized = false;
bool vt_Combined = false;
void World::Update(FrameResource* resource, ID3D12Device* device, GameTimer& timer, int2 screenSize, const Math::Vector3& moveDir, bool isMovingWorld)
{
	camMove->Run(timer.DeltaTime());
	mainCamera->SetLens(0.333333 * MathHelper::Pi, (float)screenSize.x / (float)screenSize.y, 0.3, 1000);
	/*if (!testTex)
	{
		testTex = ObjectPtr<ITexture>::MakePtr(new Texture(device, "Resource/testTex.vtex", TextureDimension::Tex2D, 20, 0));
		pbrMat->SetEmissionTexture(testTex.CastTo<ITexture>());
		pbrMat->UpdateMaterialToBuffer();

	}*/
	if (Input::IsKeyPressed(KeyCode::Ctrl))
	{
		sunTrans->SetForward(-mainCamera->GetLook());
		sunTrans->SetUp(mainCamera->GetUp());
		sunTrans->SetRight(-mainCamera->GetRight());
	}
	if (Input::IsKeyDown(KeyCode::Space))
	{
		testLightTransform->SetRight(mainCamera->GetRight());
		testLightTransform->SetUp(mainCamera->GetUp());
		testLightTransform->SetForward(mainCamera->GetLook());
		testLightTransform->SetPosition(mainCamera->GetPosition());
	}

}

void World::PrepareUpdateJob(
	JobBucket* bucket, FrameResource* resource, ID3D12Device* device, GameTimer& timer, int2 screenSize,
	Math::Vector3& moveDir,
	bool& isMovingWorld,
	std::vector<JobHandle>& mainDependedTasks)
{
	int3 blockOffset = { 0,0,0 };
	isMovingWorld = false;
	if (mainCamera->GetPosition().GetX() > BLOCK_SIZE)
	{
		blockOffset.x += 1;
		isMovingWorld = true;
	}
	else if (mainCamera->GetPosition().GetX() < -BLOCK_SIZE)
	{
		blockOffset.x -= 1;
		isMovingWorld = true;
	}
	if (mainCamera->GetPosition().GetY() > BLOCK_SIZE)
	{
		blockOffset.y += 1;
		isMovingWorld = true;
	}
	else if (mainCamera->GetPosition().GetY() < -BLOCK_SIZE)
	{
		blockOffset.y -= 1;
		isMovingWorld = true;
	}
	if (mainCamera->GetPosition().GetZ() > BLOCK_SIZE)
	{
		blockOffset.z += 1;
		isMovingWorld = true;
	}
	else if (mainCamera->GetPosition().GetZ() < -BLOCK_SIZE)
	{
		blockOffset.z -= 1;
		isMovingWorld = true;
	}
	double3 moveDirDouble = double3(blockOffset.x, blockOffset.y, blockOffset.z) * -BLOCK_SIZE;
	moveDir = Vector3(moveDirDouble.x, moveDirDouble.y, moveDirDouble.z);

	if (grpRenderer)
	{
		if (isMovingWorld)
		{
			grpRenderer->SetWorldMoving(device, moveDirDouble);
		}
		grpRenderer->MoveTheWorld(
			device, resource, bucket);
	}
	if (isMovingWorld)
	{
		int3* bIndex = &blockIndex;
		mainDependedTasks.push_back(
			bucket->GetTask(nullptr, 0, [=]()->void
				{
					auto& cameras = GetCameras();
					for (auto ite = cameras.begin(); ite != cameras.end(); ++ite)
					{
						Vector3 pos = (*ite)->GetPosition() + moveDir;
						(*ite)->SetPosition(pos);
					}

				})
		);
		Transform::MoveTheWorld(bIndex, blockOffset, moveDirDouble, bucket, mainDependedTasks);
	}
}

#undef MAXIMUM_RENDER_OBJECT