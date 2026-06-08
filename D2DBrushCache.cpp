// D2DBrushCache.cpp
#include "D2DBrushCache.h"
#include "D2DDevice.h"


namespace d2d
{
	C_D2D_BRUSH_CACHE::C_D2D_BRUSH_CACHE()
		: m_pOwner(nullptr)
	{
	}

	C_D2D_BRUSH_CACHE::~C_D2D_BRUSH_CACHE()
	{
		Shutdown();
	}

	bool C_D2D_BRUSH_CACHE::Initialize(C_D2D_DEVICE* _pOwner)
	{
		if (_pOwner == nullptr) { return false; }
		m_pOwner = _pOwner;
		return true;
	}

	void C_D2D_BRUSH_CACHE::Shutdown()
	{
		m_mapBrushes.clear();
		m_pOwner = nullptr;
	}

	void C_D2D_BRUSH_CACHE::OnDeviceLost()
	{
		// 잃은 디바이스 소속 브러시는 무효 → 비우고 재생성 유도.
		m_mapBrushes.clear();
	}

	ID2D1SolidColorBrush* C_D2D_BRUSH_CACHE::GetBrush(Color _argb)
	{
		const auto it = m_mapBrushes.find(_argb);
		if (it != m_mapBrushes.end()) { return it->second.Get(); }

		if (m_pOwner == nullptr) { return nullptr; }
		ID2D1DeviceContext* pDC = m_pOwner->GetResourceDC();
		if (pDC == nullptr) { return nullptr; }

		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBrush;
		const HRESULT hr = pDC->CreateSolidColorBrush(ToColorF(_argb), pBrush.GetAddressOf());
		if (FAILED(hr)) { return nullptr; }

		ID2D1SolidColorBrush* pRet = pBrush.Get();
		m_mapBrushes.emplace(_argb, std::move(pBrush));
		return pRet;
	}
}
