// D2DText.cpp
#include "D2DText.h"
#include "D2DDevice.h"


namespace d2d
{
	C_D2D_TEXT::C_D2D_TEXT()
		: m_pOwner(nullptr)
	{
	}

	C_D2D_TEXT::~C_D2D_TEXT()
	{
		Shutdown();
	}

	bool C_D2D_TEXT::Initialize(C_D2D_DEVICE* _pOwner)
	{
		if (_pOwner == nullptr) { return false; }
		m_pOwner = _pOwner;
		return true;
	}

	void C_D2D_TEXT::Shutdown()
	{
		m_mapFormats.clear();
		m_pOwner = nullptr;
	}

	IDWriteTextFormat* C_D2D_TEXT::GetFormat(LPCWSTR _pFont, float _fSize, DWRITE_FONT_WEIGHT _weight)
	{
		if (m_pOwner == nullptr || _pFont == nullptr) { return nullptr; }

		// 캐시 키 = "폰트|크기|굵기"
		std::wstring sKey(_pFont);
		sKey += L'|';
		sKey += std::to_wstring(static_cast<int>(_fSize * 100.0f));
		sKey += L'|';
		sKey += std::to_wstring(static_cast<int>(_weight));

		const auto it = m_mapFormats.find(sKey);
		if (it != m_mapFormats.end()) { return it->second.Get(); }

		IDWriteFactory* pDW = m_pOwner->GetDWrite();
		if (pDW == nullptr) { return nullptr; }

		Microsoft::WRL::ComPtr<IDWriteTextFormat> pFormat;
		const HRESULT hr = pDW->CreateTextFormat(_pFont, nullptr,
			_weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			_fSize, L"ko-kr", pFormat.GetAddressOf());
		if (FAILED(hr)) { return nullptr; }

		IDWriteTextFormat* pRet = pFormat.Get();
		m_mapFormats.emplace(std::move(sKey), std::move(pFormat));
		return pRet;
	}

	//------------------------------------------------------------------------------------------------
	// IDWriteTextLayout 으로 실제 픽셀 크기 측정. 라벨 배치/히트테스트용.
	//------------------------------------------------------------------------------------------------
	D2D1_SIZE_F C_D2D_TEXT::Measure(LPCWSTR _pText, IDWriteTextFormat* _pFormat, float _fMaxWidth)
	{
		D2D1_SIZE_F size{ 0.0f, 0.0f };
		if (m_pOwner == nullptr || _pText == nullptr || _pFormat == nullptr) { return size; }

		IDWriteFactory* pDW = m_pOwner->GetDWrite();
		if (pDW == nullptr) { return size; }

		const float fMaxW = (_fMaxWidth > 0.0f) ? _fMaxWidth : 100000.0f;
		Microsoft::WRL::ComPtr<IDWriteTextLayout> pLayout;
		const HRESULT hr = pDW->CreateTextLayout(_pText, static_cast<UINT32>(::wcslen(_pText)),
			_pFormat, fMaxW, 100000.0f, pLayout.GetAddressOf());
		if (FAILED(hr)) { return size; }

		DWRITE_TEXT_METRICS tm{};
		if (SUCCEEDED(pLayout->GetMetrics(&tm)))
		{
			size.width = tm.widthIncludingTrailingWhitespace;
			size.height = tm.height;
		}
		return size;
	}
}
