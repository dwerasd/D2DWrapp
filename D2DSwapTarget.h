// D2DSwapTarget.h: 창별 스왑체인 + Direct2D 디바이스 컨텍스트(렌더 타겟)
#pragma once

#include "D2DDef.h"


namespace d2d
{
	class C_D2D_DEVICE;

	// 창(HWND)마다 1개. flip-model 스왑체인 + D2D DeviceContext(타겟=백버퍼 비트맵).
	// 공유 C_D2D_DEVICE 위에 만들어진다. 디바이스 로스 시 RecreateAfterDeviceLost 로 재생성.
	class C_D2D_SWAP_TARGET
	{
	private:
		C_D2D_DEVICE* m_pOwner;		// 공유 디바이스(소유 아님)

		Microsoft::WRL::ComPtr<IDXGISwapChain1>     m_pSwapChain;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext>  m_pDC;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1>        m_pTargetBitmap;
		HANDLE m_hWaitable;			// 프레임 지연 대기 핸들(저지연 페이싱)

		HWND  m_hWnd;
		UINT  m_uWidth;
		UINT  m_uHeight;
		float m_fDpi;
		bool  m_bInDraw;

		bool createSwapChain_();
		bool createTargetBitmap_();	// 백버퍼 -> DXGI 표면 -> D2D 비트맵 -> SetTarget
		void releaseTargetBitmap_();

	public:
		C_D2D_SWAP_TARGET();
		~C_D2D_SWAP_TARGET();

		bool Initialize(C_D2D_DEVICE* _pOwner, HWND _hWnd);
		void Shutdown();

		void Resize(UINT _uWidth, UINT _uHeight);	// WM_SIZE
		void SetDpi(float _fDpi);					// WM_DPICHANGED

		// 프레임. BeginDraw 는 waitable 대기 후 BeginDraw. EndDraw 는 Present 후 반환검사.
		// EndDraw 가 false 면 디바이스 로스 — 소비자는 Owner->HandleDeviceLost() +
		// 전 SWAP_TARGET RecreateAfterDeviceLost() 후 다음 프레임 재draw.
		bool BeginDraw();
		bool EndDraw(UINT _uSync);

		void RecreateAfterDeviceLost();	// 디바이스 재생성 후 스왑체인/타겟 다시 만들기

		ID2D1DeviceContext* GetDC()     const { return m_pDC.Get(); }
		UINT  GetWidth()  const { return m_uWidth; }
		UINT  GetHeight() const { return m_uHeight; }
		HWND  GetHWND()   const { return m_hWnd; }
	};
}
