// D2DSwapTarget.cpp
#include "D2DSwapTarget.h"
#include "D2DDevice.h"

#include <algorithm>	// std::max


namespace d2d
{
	C_D2D_SWAP_TARGET::C_D2D_SWAP_TARGET()
		: m_pOwner(nullptr)
		, m_hWaitable(nullptr)
		, m_hWnd(nullptr)
		, m_uWidth(0)
		, m_uHeight(0)
		, m_fDpi(96.0f)
		, m_bInDraw(false)
	{
	}

	C_D2D_SWAP_TARGET::~C_D2D_SWAP_TARGET()
	{
		Shutdown();
	}

	//------------------------------------------------------------------------------------------------
	// flip-model 스왑체인 — BGRA, 2버퍼, FLIP_DISCARD, waitable(저지연 페이싱).
	//------------------------------------------------------------------------------------------------
	bool C_D2D_SWAP_TARGET::createSwapChain_()
	{
		if (m_pOwner == nullptr || m_hWnd == nullptr) { return false; }

		// 공유 디바이스의 DXGI 팩토리 획득.
		Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
		HRESULT hr = m_pOwner->GetDxgiDevice()->GetAdapter(pAdapter.GetAddressOf());
		if (FAILED(hr)) { return false; }

		Microsoft::WRL::ComPtr<IDXGIFactory2> pFactory;
		hr = pAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(pFactory.GetAddressOf()));
		if (FAILED(hr)) { return false; }

		DXGI_SWAP_CHAIN_DESC1 desc{};
		desc.Width = m_uWidth;
		desc.Height = m_uHeight;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 2;
		desc.Scaling = DXGI_SCALING_NONE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		hr = pFactory->CreateSwapChainForHwnd(m_pOwner->GetD3DDevice(), m_hWnd,
			&desc, nullptr, nullptr, m_pSwapChain.GetAddressOf());
		if (FAILED(hr)) { return false; }

		// ALT+ENTER 전체화면 토글 차단(차트 앱).
		pFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

		// 프레임 지연 1 + 대기 핸들(저지연).
		Microsoft::WRL::ComPtr<IDXGISwapChain2> pSwap2;
		if (SUCCEEDED(m_pSwapChain.As(&pSwap2)))
		{
			pSwap2->SetMaximumFrameLatency(1);
			m_hWaitable = pSwap2->GetFrameLatencyWaitableObject();
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// 백버퍼 -> DXGI 표면 -> D2D 비트맵 -> 디바이스 컨텍스트 타겟 설정.
	//------------------------------------------------------------------------------------------------
	bool C_D2D_SWAP_TARGET::createTargetBitmap_()
	{
		if (m_pDC == nullptr)
		{
			const HRESULT hr = m_pOwner->GetD2DDevice()->CreateDeviceContext(
				D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_pDC.GetAddressOf());
			if (FAILED(hr)) { return false; }
		}

		Microsoft::WRL::ComPtr<IDXGISurface> pSurface;
		HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(IDXGISurface),
			reinterpret_cast<void**>(pSurface.GetAddressOf()));
		if (FAILED(hr)) { return false; }

		const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
			m_fDpi, m_fDpi);

		hr = m_pDC->CreateBitmapFromDxgiSurface(pSurface.Get(), &props, m_pTargetBitmap.GetAddressOf());
		if (FAILED(hr)) { return false; }

		m_pDC->SetTarget(m_pTargetBitmap.Get());
		m_pDC->SetDpi(m_fDpi, m_fDpi);
		return true;
	}

	void C_D2D_SWAP_TARGET::releaseTargetBitmap_()
	{
		if (m_pDC != nullptr) { m_pDC->SetTarget(nullptr); }
		m_pTargetBitmap.Reset();
	}

	bool C_D2D_SWAP_TARGET::Initialize(C_D2D_DEVICE* _pOwner, HWND _hWnd)
	{
		if (_pOwner == nullptr || _hWnd == nullptr) { return false; }
		m_pOwner = _pOwner;
		m_hWnd = _hWnd;

		RECT rc{};
		::GetClientRect(m_hWnd, &rc);
		m_uWidth = static_cast<UINT>((std::max)(1L, rc.right - rc.left));
		m_uHeight = static_cast<UINT>((std::max)(1L, rc.bottom - rc.top));
		m_fDpi = static_cast<float>(::GetDpiForWindow(m_hWnd));
		if (m_fDpi < 96.0f) { m_fDpi = 96.0f; }

		if (!createSwapChain_()) { return false; }
		if (!createTargetBitmap_()) { return false; }
		return true;
	}

	void C_D2D_SWAP_TARGET::Shutdown()
	{
		releaseTargetBitmap_();
		m_pDC.Reset();
		m_pSwapChain.Reset();
		m_hWaitable = nullptr;
		m_pOwner = nullptr;
	}

	//------------------------------------------------------------------------------------------------
	// 리사이즈 — 타겟 해제 후 ResizeBuffers, 타겟 재생성.
	//------------------------------------------------------------------------------------------------
	void C_D2D_SWAP_TARGET::Resize(UINT _uWidth, UINT _uHeight)
	{
		if (m_pSwapChain == nullptr) { return; }
		const UINT w = (std::max<UINT>)(1u, _uWidth);
		const UINT h = (std::max<UINT>)(1u, _uHeight);
		if (w == m_uWidth && h == m_uHeight) { return; }
		m_uWidth = w; m_uHeight = h;

		releaseTargetBitmap_();
		const HRESULT hr = m_pSwapChain->ResizeBuffers(0, m_uWidth, m_uHeight,
			DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
		if (SUCCEEDED(hr)) { createTargetBitmap_(); }
		// 디바이스 로스(DEVICE_REMOVED)면 소비자가 다음 프레임 EndDraw 에서 감지/복구.
	}

	void C_D2D_SWAP_TARGET::SetDpi(float _fDpi)
	{
		if (_fDpi < 96.0f) { _fDpi = 96.0f; }
		if (_fDpi == m_fDpi) { return; }
		m_fDpi = _fDpi;
		if (m_pDC != nullptr) { m_pDC->SetDpi(m_fDpi, m_fDpi); }
	}

	bool C_D2D_SWAP_TARGET::BeginDraw()
	{
		if (m_pDC == nullptr || m_pTargetBitmap == nullptr) { return false; }
		// 저지연 페이싱 — 다음 프레임 준비될 때까지 대기(CPU 양보).
		if (m_hWaitable != nullptr) { ::WaitForSingleObjectEx(m_hWaitable, 1000, TRUE); }
		m_pDC->BeginDraw();
		m_bInDraw = true;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	// EndDraw + Present. 디바이스 로스 감지 시 false 반환(소비자가 복구).
	//------------------------------------------------------------------------------------------------
	bool C_D2D_SWAP_TARGET::EndDraw(UINT _uSync)
	{
		if (!m_bInDraw) { return true; }
		m_bInDraw = false;

		const HRESULT hrEnd = m_pDC->EndDraw();
		if (hrEnd == D2DERR_RECREATE_TARGET) { return false; }

		const HRESULT hrPresent = m_pSwapChain->Present(_uSync, 0);
		if (hrPresent == DXGI_ERROR_DEVICE_REMOVED || hrPresent == DXGI_ERROR_DEVICE_RESET)
		{
			return false;
		}
		return SUCCEEDED(hrEnd);
	}

	//------------------------------------------------------------------------------------------------
	// 디바이스 로스 복구 — Owner->HandleDeviceLost() 후 호출. 스왑체인/타겟/DC 재생성.
	//------------------------------------------------------------------------------------------------
	void C_D2D_SWAP_TARGET::RecreateAfterDeviceLost()
	{
		releaseTargetBitmap_();
		m_pDC.Reset();			// 컨텍스트는 잃은 디바이스 소속 → 폐기
		m_pSwapChain.Reset();
		m_hWaitable = nullptr;

		if (createSwapChain_()) { createTargetBitmap_(); }
	}
}
