//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "d3dUtil.h"

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

	m_Camera = std::make_unique<Bruce::Camera>();
	m_Camera->CreateView(Vector3(0, 10, 10), Vector3::Zero, Vector3::UnitY);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */

	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<DirectX::Mouse>();
	m_mouse->SetWindow(window);

	m_obj1_world = Matrix::Identity;
	m_obj2_world = Matrix::Identity;
	m_obj3_world = Matrix::Identity;

	m_obj1_world.Translation(Vector3(-2, 1, -1));
	m_obj2_world.Translation(Vector3(2, -1,  1));
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

static const float CAMERA_MOVE_SPEED = 10.0f;
static const float CAMERA_ROTATE_SPEED = 0.25f;

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	float elapsedTime = float(timer.GetElapsedSeconds());

	{	// keyboard
		auto kb = m_keyboard->GetState();
		float moveDelta = CAMERA_MOVE_SPEED * elapsedTime;
		if (kb.W)
		{
			m_Camera->Walk( moveDelta);
		}
		if (kb.S) 
		{
			m_Camera->Walk(-moveDelta);
		}
		if (kb.A) 
		{
			m_Camera->Strafe(-moveDelta);
		}
		if (kb.D) 
		{
			m_Camera->Strafe( moveDelta);
		}
		if (kb.Q)
		{
			m_Camera->Fly(moveDelta);
		}
		if (kb.E)
		{
			m_Camera->Fly(-moveDelta);
		}
	}

	{	// mouse
		auto mouse = m_mouse->GetState();
		if (mouse.positionMode == Mouse::MODE_RELATIVE)
		{
			Vector3 delta = Vector3(float(mouse.x), float(mouse.y), 0.f) * CAMERA_ROTATE_SPEED;
			Vector3 deltaRadian(XMConvertToRadians(delta.x), XMConvertToRadians(delta.y), 0);

			m_Camera->Pitch(-deltaRadian.y);
			m_Camera->RotateY(-deltaRadian.x);
		}

		m_mouse->SetMode(mouse.rightButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
	}
	
	m_Camera->UpdateViewMatrix();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

	m_obj1->Draw(m_obj1_world, m_Camera->GetView(), m_Camera->GetProj(), Colors::AliceBlue);
	m_obj2->Draw(m_obj2_world, m_Camera->GetView(), m_Camera->GetProj(), Colors::Crimson);
	m_obj3->Draw(m_obj3_world, m_Camera->GetView(), m_Camera->GetProj(), Colors::CadetBlue);

	// unbound resources & set swap chain render target
	auto rt = m_deviceResources->GetRenderTargetView();
	context->OMSetRenderTargets(1, &rt, nullptr);

	// post process
	
	{	// sobel
		{	// Detect edge
			context->CSSetShader(m_sobel_cs.Get(), nullptr, 0);
			context->CSSetShaderResources(0, 1, m_RT_SRV.GetAddressOf());
			context->CSSetUnorderedAccessViews(0, 1, m_sobel_edge_uav.GetAddressOf(), nullptr);

			UINT tGroupX = UINT(ceil(float(m_deviceResources->GetOutputSize().right) / 16.0f));
			UINT tGroupY = UINT(ceil(float(m_deviceResources->GetOutputSize().bottom) / 16.0f));
			context->Dispatch(tGroupX, tGroupY, 1);

			// unbound resources
			ID3D11ShaderResourceView* null_srvs[] = { nullptr };
			context->CSSetShaderResources(0, _countof(null_srvs), null_srvs);

			ID3D11UnorderedAccessView* nullUAV = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

			context->CSSetShader(nullptr, nullptr, 0);
		}
		
		{	// Draw edge
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			context->VSSetShader(m_sobel_vs.Get(), nullptr, 0);
			context->PSSetShader(m_sobel_ps.Get(), nullptr, 0);

			ID3D11ShaderResourceView* srvs[] = { m_RT_SRV.Get(), m_sobel_edge_srv.Get() };
			context->PSSetShaderResources(0, _countof(srvs), srvs);
			
			auto sampler = m_states->PointClamp();
			context->PSSetSamplers(0, 1, &sampler);
			
			context->Draw(6, 0);

			// unbound resources
			ID3D11ShaderResourceView* null_srvs[] = { nullptr, nullptr };
			context->PSSetShaderResources(0, _countof(null_srvs), null_srvs);
			context->PSSetShader(nullptr, nullptr, 0);
			context->VSSetShader(nullptr, nullptr, 0);
		}
	}

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

	context->ClearRenderTargetView(m_RTV.Get(), Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	context->OMSetRenderTargets(1, m_RTV.GetAddressOf(), depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
	auto device = m_deviceResources->GetD3DDevice();
	auto context = m_deviceResources->GetD3DDeviceContext();

	m_states = std::make_unique<DirectX::CommonStates>(device);

	{	// shaders
		auto vs = d3dUtil::CompileShader(L"Composite.hlsl", nullptr, "VS", "vs_5_0");
		DX::ThrowIfFailed(
			device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, m_sobel_vs.ReleaseAndGetAddressOf()));

		auto ps = d3dUtil::CompileShader(L"Composite.hlsl", nullptr, "PS", "ps_5_0");
		DX::ThrowIfFailed(
			device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, m_sobel_ps.ReleaseAndGetAddressOf()));

		auto cs = d3dUtil::CompileShader(L"FindOutline.hlsl", nullptr, "CS", "cs_5_0");
		DX::ThrowIfFailed(
			device->CreateComputeShader(cs->GetBufferPointer(), cs->GetBufferSize(), nullptr, m_sobel_cs.ReleaseAndGetAddressOf()));
	}

	m_obj1 = GeometricPrimitive::CreateTeapot(context, 4.0f);
	m_obj2 = GeometricPrimitive::CreateCone(context, 5.0f, 3.0f);
	m_obj3 = GeometricPrimitive::CreateTetrahedron(context, 3);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
	auto width = m_deviceResources->GetOutputSize().right;
	auto height = m_deviceResources->GetOutputSize().bottom;
	auto device = m_deviceResources->GetD3DDevice();

	
	{	// render scene render target & use as shader input
		CD3D11_TEXTURE2D_DESC desc(
			m_deviceResources->GetBackBufferFormat(),
			width, height,
			1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE
		);

		DX::ThrowIfFailed(
			device->CreateTexture2D(&desc, nullptr, m_buffer_RT.ReleaseAndGetAddressOf()));

		// RTV
		DX::ThrowIfFailed(
			device->CreateRenderTargetView(m_buffer_RT.Get(), nullptr, m_RTV.ReleaseAndGetAddressOf()));

		// SRV
		DX::ThrowIfFailed(
			device->CreateShaderResourceView(m_buffer_RT.Get(), nullptr, m_RT_SRV.ReleaseAndGetAddressOf()));
	}
	
	{	// sobel edge texture resource
		CD3D11_TEXTURE2D_DESC desc(
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			width, height, 1, 1, 
			D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);

		DX::ThrowIfFailed(
			device->CreateTexture2D(&desc, nullptr, m_sobel_edge.ReleaseAndGetAddressOf()));
	}

	{	// Edge UAV (detect edge & save)
		DX::ThrowIfFailed(		// Unordered Access View can be created with null description.
			device->CreateUnorderedAccessView(m_sobel_edge.Get(), nullptr, m_sobel_edge_uav.ReleaseAndGetAddressOf()));
	}

	{	// Edge SRV (use edge as shader input)
		DX::ThrowIfFailed(
			device->CreateShaderResourceView(m_sobel_edge.Get(), nullptr, m_sobel_edge_srv.ReleaseAndGetAddressOf()));
	}

	m_Camera->CreateProj(XM_PIDIV4, float(width) / float(height), 0.01f, 1000.0f);
}

void Game::OnDeviceLost()
{
	m_states.reset();
	m_obj1.reset();
	m_obj2.reset();
	m_obj3.reset();

	m_sobel_vs.Reset();
	m_sobel_ps.Reset();
	m_sobel_cs.Reset();

	m_buffer_RT.Reset();
	m_RTV.Reset();
	m_RT_SRV.Reset();

	m_sobel_edge.Reset();
	m_sobel_edge_uav.Reset();
	m_sobel_edge_srv.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
