//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "GeometricPrimitive.h"
#include "SimpleMath.h"
#include "CommonStates.h"
#include "Camera.h"
#include "Keyboard.h"
#include "Mouse.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;


	//
	std::unique_ptr<DirectX::Keyboard> m_keyboard;
	std::unique_ptr<DirectX::Mouse> m_mouse;

	std::unique_ptr<Bruce::Camera> m_Camera;
	std::unique_ptr<DirectX::CommonStates> m_states;

	// 
	std::unique_ptr<DirectX::GeometricPrimitive> m_obj1;
	DirectX::SimpleMath::Matrix m_obj1_world;

	std::unique_ptr<DirectX::GeometricPrimitive> m_obj2;
	DirectX::SimpleMath::Matrix m_obj2_world;

	std::unique_ptr<DirectX::GeometricPrimitive> m_obj3;
	DirectX::SimpleMath::Matrix m_obj3_world;

	// sobel effects
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_sobel_vs;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_sobel_ps;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_sobel_cs;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_buffer_RT;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_RTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_RT_SRV;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_sobel_edge;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_sobel_edge_uav;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sobel_edge_srv;

};