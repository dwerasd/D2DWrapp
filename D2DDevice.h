// D2DDevice.h: 공유 D3D11 + Direct2D 디바이스 (디바이스 독립자원 + 로스 복구)
#pragma once

#include "D2DDef.h"


namespace d2d
{
	// 애플리케이션당 1개. 전 차트 창(C_D2D_SWAP_TARGET)이 이 디바이스를 공유한다.
	// 디바이스 독립자원(팩토리/DWrite)은 생성 1회, 디바이스 종속자원(D3D11/D2D 디바이스 + 리소스 DC)은
	// 디바이스 로스 시 재생성한다.
	class C_D2D_DEVICE
	{
	private:
		// 디바이스 독립(로스에도 생존)
		Microsoft::WRL::ComPtr<ID2D1Factory1>        m_pD2DFactory;
		Microsoft::WRL::ComPtr<IDWriteFactory>       m_pDWriteFactory;

		// 디바이스 종속(로스 시 재생성)
		Microsoft::WRL::ComPtr<ID3D11Device>         m_pD3DDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>  m_pD3DContext;
		Microsoft::WRL::ComPtr<IDXGIDevice1>         m_pDxgiDevice;
		Microsoft::WRL::ComPtr<ID2D1Device>          m_pD2DDevice;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext>   m_pResourceDC;	// 공유 브러시 생성용(타겟 없음)

		bool m_bInitialized;

		bool createDeviceIndependent_();	// 팩토리/DWrite (1회)
		bool createDeviceDependent_();		// D3D11/D2D 디바이스 + 리소스 DC (재생성 가능)
		void discardDeviceDependent_();

	public:
		C_D2D_DEVICE();
		~C_D2D_DEVICE();

		bool Initialize();
		void Shutdown();

		// 디바이스 로스 복구: 종속자원 폐기 후 재생성. 성공 시 true.
		// (각 C_D2D_SWAP_TARGET 은 별도로 RecreateAfterDeviceLost 호출 필요.)
		bool HandleDeviceLost();

		bool IsInitialized() const { return m_bInitialized; }

		ID2D1Factory1*      GetFactory()    const { return m_pD2DFactory.Get(); }
		IDWriteFactory*     GetDWrite()     const { return m_pDWriteFactory.Get(); }
		ID3D11Device*       GetD3DDevice()  const { return m_pD3DDevice.Get(); }
		IDXGIDevice1*       GetDxgiDevice() const { return m_pDxgiDevice.Get(); }
		ID2D1Device*        GetD2DDevice()  const { return m_pD2DDevice.Get(); }
		ID2D1DeviceContext* GetResourceDC() const { return m_pResourceDC.Get(); }	// 브러시 캐시용
	};
}
