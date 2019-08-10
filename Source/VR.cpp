
//#include <Urho3D/Engine/Application.h>
//#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/StaticModelGroup.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/Node.h>
#include "VR.h"

namespace Urho3D {

	VR::VR(Context* context) :
		Object(context)
	{
		SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(VR, HandleUpdate));
		SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(VR, HandlePostRender));
		m_pHMD = NULL;

		
	}

	void VR::RegisterObject(Context* context)
	{
		context->RegisterSubsystem(new VR(context));

	}

	bool VR::InitializeVR(Node* referenceNode)
	{
		vr::EVRInitError eError = vr::VRInitError_None;
		m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
		if (eError != vr::VRInitError_None)
		{
			m_pHMD = NULL;
			URHO3D_LOGERROR(vr::VR_GetVRInitErrorAsEnglishDescription(eError));
			return false;
		}
		if (!vr::VRCompositor())
		{
			m_pHMD = NULL;
			URHO3D_LOGERROR("Compositor initialization failed. See log file for details");
			return false;
		}

		if (!m_pHMD)
			return false;


		SetReferenceNode(referenceNode);

		

		uint32_t m_nRenderWidth;
		uint32_t m_nRenderHeight;
		m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);
		//the second rendertexture to have SetSize() called will have shadows...
		leftRenderTexture = new Texture2D(context_);
		leftRenderTexture->SetSize(m_nRenderWidth, m_nRenderHeight, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
		leftRenderTexture->SetFilterMode(FILTER_BILINEAR);
		RenderSurface* leftSurface = leftRenderTexture->GetRenderSurface();
		leftSurface->SetUpdateMode(SURFACE_UPDATEALWAYS);
		SharedPtr<Viewport> leftrttViewport(new Viewport(context_, leftCamera_->GetScene(), leftCamera_));
		leftSurface->SetViewport(0, leftrttViewport);
		rightRenderTexture = new Texture2D(context_);
		rightRenderTexture->SetSize(m_nRenderWidth, m_nRenderHeight, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
		rightRenderTexture->SetFilterMode(FILTER_BILINEAR);
		RenderSurface* rightSurface = rightRenderTexture->GetRenderSurface();
		rightSurface->SetUpdateMode(SURFACE_UPDATEALWAYS);
		SharedPtr<Viewport> rightrttViewport(new Viewport(context_, rightCamera_->GetScene(), rightCamera_));
		rightSurface->SetViewport(0, rightrttViewport);

		UpdateNodes();

		return true;
	}


	void VR::HandleUpdate(StringHash eventType, VariantMap& eventData)
	{
		if (m_pHMD) {
			UpdateHMDMatrixPose(); //get tracking data from OpenVR
								   // Process SteamVR controller state
			for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
			{
				vr::VRControllerState_t state;
				if (m_pHMD->GetControllerState(unDevice, &state, sizeof(state)))
				{
					//m_rbShowTrackedDevice[unDevice] = state.ulButtonPressed == 0;toggle visibility of controller
				}
			}

			UpdateNodes();
		}
	}

	//get tracking data from OpenVR
	void VR::UpdateHMDMatrixPose()
	{
		if (!m_pHMD)
			return;
		vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
	}

	void VR::HandlePostRender(StringHash eventType, VariantMap& eventData)
	{
		if (!m_pHMD)
			return;
		vr::EVRCompositorError eError = vr::VRCompositorError_None;
		vr::Texture_t leftEyeTexture = { (leftRenderTexture->GetGPUObject()), vr::TextureType_DirectX, vr::ColorSpace_Auto };
		eError = vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		if (eError != vr::VRCompositorError_None)
		{
			URHO3D_LOGERROR("LeftEyeError");
			URHO3D_LOGERROR(ea::to_string((unsigned)eError));
		}
		vr::Texture_t rightEyeTexture = { (rightRenderTexture->GetGPUObject()), vr::TextureType_DirectX, vr::ColorSpace_Auto };
		eError = vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
		if (eError != vr::VRCompositorError_None)
		{
			URHO3D_LOGERROR("RightEyeError");
			URHO3D_LOGERROR(ea::to_string((unsigned)eError));
		}
		
	}

	void VR::FixedUpdate(float timeStep)
	{
	}

	Matrix3x4 VR::GetHeadTransform()
	{
		if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
		{
			return Matrix3x4(ConvertHmdMatrix34_tToMatrix4(m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking));
		}
		return Matrix3x4();
	}

	Matrix3x4 VR::GetHandTransform(bool isRightHand)
	{
		uint32_t handIndex = (uint32_t)(m_pHMD->GetTrackedDeviceIndexForControllerRole(isRightHand ? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand));
		if (handIndex < vr::k_unMaxTrackedDeviceCount && m_rTrackedDevicePose[handIndex].bPoseIsValid)
		{
			return Matrix3x4(ConvertHmdMatrix34_tToMatrix4(m_rTrackedDevicePose[handIndex].mDeviceToAbsoluteTracking));
		}
		return Matrix3x4();
	}

	bool VR::GetControllerState(bool isRightHand, vr::VRControllerState_t *pControllerState)
	{
		TrackedDeviceIndex_t handIndex = m_pHMD->GetTrackedDeviceIndexForControllerRole(isRightHand ? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand);
		vr::VRControllerState_t state = vr::VRControllerState_t();
		if (m_pHMD->GetControllerState(handIndex, &state, sizeof(state)))
		{
			*pControllerState = state;
			return true;
		}
		return false;
	}

	void VR::UpdateNodes()
	{
		headNode_->SetTransform(Matrix3x4(GetHeadTransform()));
		leftHandNode_->SetTransform(Matrix3x4(GetHandTransform(false)));
		rightHandNode_->SetTransform(Matrix3x4(GetHandTransform(true)));


		Matrix4 leftEyePos = ConvertHmdMatrix34_tToMatrix4(m_pHMD->GetEyeToHeadTransform(vr::Eye_Left));
		Matrix4 rightEyePos = ConvertHmdMatrix34_tToMatrix4(m_pHMD->GetEyeToHeadTransform(vr::Eye_Right));

		//update projection in case it changes over time.
		leftCamera_->SetProjection((ConvertHmdMatrix44_tToMatrix4(m_pHMD->GetProjectionMatrix(vr::Eye_Left, 0.1f, 900.f))*-1.0f));
		rightCamera_->SetProjection((ConvertHmdMatrix44_tToMatrix4(m_pHMD->GetProjectionMatrix(vr::Eye_Right, 0.1f, 900.f))*-1.0f));


		leftCamera_->GetNode()->SetTransform(Matrix3x4(leftEyePos));
		rightCamera_->GetNode()->SetTransform(Matrix3x4(rightEyePos));


		leftCamera_->GetNode()->SetRotation(Quaternion(0, 0, 180));
		rightCamera_->GetNode()->SetRotation(Quaternion(0, 0, 180));
	}

	// not working
	/*void VR::TriggerHapticPulse(bool isRightHand, unsigned short duration)
	{
		unsigned int axisId = (int)vr::k_EButton_SteamVR_Touchpad - (int)vr::k_EButton_Axis0;
		//URHO3D_LOGINFO("Haptic Pulse sent strength " + String(axisId));
		m_pHMD->TriggerHapticPulse(isRightHand ? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand, axisId, (char)duration);
	}*/

	Matrix4 VR::ConvertHmdMatrix34_tToMatrix4(const vr::HmdMatrix34_t &matPose) //Converts a SteamVR matrix to urho3d matrix class
	{
		return Matrix4(
			matPose.m[0][0], matPose.m[0][1], -matPose.m[0][2], matPose.m[0][3],
			matPose.m[1][0], matPose.m[1][1], -matPose.m[1][2], matPose.m[1][3],
			-matPose.m[2][0], -matPose.m[2][1], matPose.m[2][2], -matPose.m[2][3],
			//1, 0, 0, 1
			matPose.m[3][0], matPose.m[3][1], matPose.m[3][2], matPose.m[3][3]
		);
	}

	Matrix4 VR::ConvertHmdMatrix44_tToMatrix4(const vr::HmdMatrix44_t &matPose) //Converts a SteamVR matrix to urho3d matrix class
	{
		return Matrix4(
			matPose.m[0][0], matPose.m[0][1], matPose.m[0][2], matPose.m[0][3],
			matPose.m[1][0], matPose.m[1][1], matPose.m[1][2], matPose.m[1][3],
			matPose.m[2][0], matPose.m[2][1], matPose.m[2][2], -matPose.m[2][3],
			//0, 0, 0, 0
			matPose.m[3][0], matPose.m[3][1], matPose.m[3][2], matPose.m[3][3]
		);
	}

	void VR::SetShowVisualControllers(bool enable)
	{
		leftHandNode_->GetComponent<StaticModel>()->SetEnabled(enable);
		rightHandNode_->GetComponent<StaticModel>()->SetEnabled(enable);
	}

	void VR::Stop()
	{
		if (m_pHMD)
		{
			vr::VR_Shutdown();
			m_pHMD = NULL;
		}
	}

	void VR::SetReferenceNode(Node* node)
	{
		referenceNode_ = node;

		headNode_ = referenceNode_->CreateChild("Head Node");
		leftHandNode_ = referenceNode_->CreateChild("Left Hand Node");
		rightHandNode_ = referenceNode_->CreateChild("Right Hand Node");


		StaticModel* leftHandstmdl = leftHandNode_->CreateComponent<StaticModel>();
		StaticModel* rightHandstmdl = rightHandNode_->CreateComponent<StaticModel>();


		leftHandstmdl->SetModel(GetSubsystem<ResourceCache>()->GetResource<Model>("Models/valve_index/valve_controller_knu_1_0_left/valve_controller_knu_1_0_left.mdl"));
		leftHandstmdl->SetMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/valve/knuckles_left.xml"));

		rightHandstmdl->SetModel(GetSubsystem<ResourceCache>()->GetResource<Model>("Models/valve_index/valve_controller_knu_1_0_right/valve_controller_knu_1_0_right.mdl"));
		rightHandstmdl->SetMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/valve/knuckles_right.xml"));

		Node* leftCameraNode_ = headNode_->CreateChild("LeftCamera");
		leftCamera_ = leftCameraNode_->CreateComponent<Camera>();
		leftCamera_->SetFarClip(900.0f);
		Node* rightCameraNode_ = headNode_->CreateChild("RightCamera");
		rightCamera_ = rightCameraNode_->CreateComponent<Camera>();
		rightCamera_->SetFarClip(900.0f);
	}

}