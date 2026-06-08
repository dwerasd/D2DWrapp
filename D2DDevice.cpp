// D2DDevice.cpp
#include "D2DDevice.h"


namespace d2d
{
	C_D2D_DEVICE::C_D2D_DEVICE()
		: m_bInitialized(false)
	{
	}

	C_D2D_DEVICE::~C_D2D_DEVICE()
	{
		Shutdown();
	}

	//------------------------------------------------------------------------------------------------
	// 디바이스 독립자원 — 팩토리/DWrite. 로스에도 생존하므로 1회만 생성.
	//------------------------------------------------------------------------------------------------
	bool C_D2D_DEVICE::createDeviceIndependent_()
	{
		if (m_pD2DFactory == nullptr)
		{
			D2D1_FACTORY_OPTIONS opt{};
			const HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
				__uuidof(ID2D1Factory1), &opt, reinterpret_cast<void**>(m_pD2DFactory.GetAddressOf()));
			if (FAILED(hr)) { return false; }
		}
		if (m_pDWriteFactory == nullptr)
		{
			const HRESULT hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_pDWriteFactory.GetAddressOf()));
			if (FAILED(hr)) { return false; }
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// 디바이스 종속자원 — D3D11(BGRA) + D2D 디바이스 + 리소스 DC(공유 브러시 생성용).
	// 로스 시 discard 후 재호출로 재생성.
	//------------------------------------------------------------------------------------------------
	bool C_D2D_DEVICE::createDeviceDependent_()
	{
		// BGRA_SUPPORT 는 D2D 상호운용 필수. 단일스레드로 오버헤드 절감.
		UINT uFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED;

		const D3D_FEATURE_LEVEL aLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_1,
		};

		// 1차: 하드웨어. 실패 시 WARP(소프트웨어) 폴백 — 견고성.
		HRESULT hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, uFlags,
			aLevels, ARRAYSIZE(aLevels), D3D11_SDK_VERSION,
			m_pD3DDevice.GetAddressOf(), nullptr, m_pD3DContext.GetAddressOf());
		if (FAILED(hr))
		{
			hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, uFlags,
				aLevels, ARRAYSIZE(aLevels), D3D11_SDK_VERSION,
				m_pD3DDevice.GetAddressOf(), nullptr, m_pD3DContext.GetAddressOf());
			if (FAILED(hr)) { return false; }
		}

		hr = m_pD3DDevice.As(&m_pDxgiDevice);
		if (FAILED(hr)) { return false; }

		hr = m_pD2DFactory->CreateDevice(m_pDxgiDevice.Get(), m_pD2DDevice.GetAddressOf());
		if (FAILED(hr)) { return false; }

		hr = m_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			m_pResourceDC.GetAddressOf());
		if (FAILED(hr)) { return false; }

		return true;
	}

	void C_D2D_DEVICE::discardDeviceDependent_()
	{
		m_pResourceDC.Reset();
		m_pD2DDevice.Reset();
		m_pDxgiDevice.Reset();
		m_pD3DContext.Reset();
		m_pD3DDevice.Reset();
	}

	bool C_D2D_DEVICE::Initialize()
	{
		if (m_bInitialized) { return true; }
		if (!createDeviceIndependent_()) { return false; }
		if (!createDeviceDependent_()) { discardDeviceDependent_(); return false; }
		m_bInitialized = true;
		return true;
	}

	void C_D2D_DEVICE::Shutdown()
	{
		discardDeviceDependent_();
		m_pDWriteFactory.Reset();
		m_pD2DFactory.Reset();
		m_bInitialized = false;
	}

	//------------------------------------------------------------------------------------------------
	// 디바이스 로스 복구 — 종속자원만 폐기 후 재생성(독립자원은 유지).
	// 호출 후 각 SWAP_TARGET 의 RecreateAfterDeviceLost 로 스왑체인/타겟을 다시 만든다.
	//------------------------------------------------------------------------------------------------
	bool C_D2D_DEVICE::HandleDeviceLost()
	{
		discardDeviceDependent_();
		return createDeviceDependent_();
	}
}
