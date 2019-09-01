
#pragma once


#include <UrhoOpenVRApi.h>


#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Input/Controls.h>
#include <openvr.h>
#include <openvr_capi.h>



namespace Urho3D {

	class Drawable;
	class Node;
	class Scene;
	class CollisionShape;

/// Virtual Reality Subsystem
class URHO_OPENVR_API VR : public Object
{
	URHO3D_OBJECT(VR, Object);

public:
	/// Construct.
	VR(Context* context);

	/// Register object factory and attributes.
	static void RegisterObject(Context* context);

	// Starts the VR subsystem
	virtual bool InitializeVR(Node* referenceNode);

	bool IsRunning() const { return isRunning_; }

	// Handle update per visual frame
	virtual void HandleUpdate(StringHash eventType, VariantMap& eventData);
	// Get tracking data from OpenVR
	virtual void UpdateHMDMatrixPose();
	// Post Render update -- we send the images to the headset after rendering
	virtual void HandlePostRender(StringHash eventType, VariantMap& eventData);
	/// Handle physics world update. Called by LogicComponent base class.
	virtual void FixedUpdate(float timeStep);
	// Converts a SteamVR matrix to urho3d matrix class
	virtual Matrix4 ConvertHmdMatrix34_tToMatrix4(const vr::HmdMatrix34_t &matPose);
	// Converts a SteamVR matrix to urho3d matrix class
	virtual Matrix4 ConvertHmdMatrix44_tToMatrix4(const vr::HmdMatrix44_t &matPose);
	
	virtual void SetShowVisualControllers(bool enable);
	
	
	
	// Cleans up the VR subsystem
	virtual void Stop();


	//set the reference node all other nodes (including head node) are referencing from.
	virtual void SetReferenceNode(Node* referenceNode);



	virtual bool GetControllerState(bool isRightHand, vr::VRControllerState_t *pControllerState);
	//virtual void TriggerHapticPulse(bool isRightHand, unsigned short duration);

	void UpdateNodes();


	Matrix3x4 GetHeadTransform();
	Matrix3x4 GetHandTransform(bool isRightHand);

	bool isRunning_ = false;

	//pointer to the OpenVR API
	vr::IVRSystem* m_pHMD;
	WeakPtr<Node> referenceNode_;
	SharedPtr<Node> headNode_;
	SharedPtr<Camera> leftCamera_;
	SharedPtr<Camera> rightCamera_;
	SharedPtr<Node> rightHandNode_;
	SharedPtr<Node> leftHandNode_;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 m_mat4HMDPose;
	SharedPtr<Texture2D> leftRenderTexture;
	SharedPtr<Texture2D> rightRenderTexture;
};

}