// D2DText.cpp
#include "D2DText.h"
#include "D2DDevice.h"

#include <algorithm>
#include <limits>


namespace d2d
{
	namespace
	{
		bool isHighSurrogate(wchar_t _ch)
		{
			return _ch >= 0xD800 && _ch <= 0xDBFF;
		}

		bool isLowSurrogate(wchar_t _ch)
		{
			return _ch >= 0xDC00 && _ch <= 0xDFFF;
		}
	}

	D2D1_SIZE_F C_D2D_TEXT_LAYOUT::Measure() const
	{
		D2D1_SIZE_F size{ 0.0f, 0.0f };
		if (m_pLayout == nullptr) { return size; }
		DWRITE_TEXT_METRICS metrics{};
		if (SUCCEEDED(m_pLayout->GetMetrics(&metrics)))
		{
			size.width = metrics.widthIncludingTrailingWhitespace;
			size.height = metrics.height;
		}
		return size;
	}

	std::vector<DWRITE_LINE_METRICS> C_D2D_TEXT_LAYOUT::GetLineMetrics() const
	{
		std::vector<DWRITE_LINE_METRICS> lines;
		if (m_pLayout == nullptr) { return lines; }
		UINT32 count = 0;
		HRESULT hr = m_pLayout->GetLineMetrics(nullptr, 0, &count);
		if (hr != E_NOT_SUFFICIENT_BUFFER || count == 0) { return lines; }
		lines.resize(count);
		hr = m_pLayout->GetLineMetrics(lines.data(), count, &count);
		if (FAILED(hr)) { lines.clear(); }
		else
		{
			lines.resize(count);
			if (m_uVisibleLineCap > 0 && lines.size() > m_uVisibleLineCap)
			{
				lines.resize(m_uVisibleLineCap);
			}
		}
		return lines;
	}

	bool C_D2D_TEXT_LAYOUT::HitTestTextPosition(UINT32 _uPosition, bool _bTrailing,
		D2D1_POINT_2F* _pPoint, DWRITE_HIT_TEST_METRICS* _pMetrics) const
	{
		if (m_pLayout == nullptr || _pPoint == nullptr) { return false; }
		DWRITE_HIT_TEST_METRICS local{};
		float x = 0.0f;
		float y = 0.0f;
		const HRESULT hr = m_pLayout->HitTestTextPosition(_uPosition, _bTrailing, &x, &y,
			_pMetrics != nullptr ? _pMetrics : &local);
		if (FAILED(hr)) { return false; }
		*_pPoint = D2D1::Point2F(x, y);
		return true;
	}

	bool C_D2D_TEXT_LAYOUT::HitTestPoint(float _fX, float _fY, UINT32* _pPosition,
		bool* _pTrailing, bool* _pInside, DWRITE_HIT_TEST_METRICS* _pMetrics) const
	{
		if (m_pLayout == nullptr || _pPosition == nullptr || _pTrailing == nullptr) { return false; }
		BOOL trailing = FALSE;
		BOOL inside = FALSE;
		DWRITE_HIT_TEST_METRICS local{};
		DWRITE_HIT_TEST_METRICS* metrics = _pMetrics != nullptr ? _pMetrics : &local;
		const HRESULT hr = m_pLayout->HitTestPoint(_fX, _fY, &trailing, &inside, metrics);
		if (FAILED(hr)) { return false; }
		*_pPosition = metrics->textPosition;
		*_pTrailing = trailing != FALSE;
		if (_pInside != nullptr) { *_pInside = inside != FALSE; }
		return true;
	}

	void C_D2D_TEXT_LAYOUT::Draw(ID2D1RenderTarget* _pTarget, ID2D1Brush* _pBrush,
		D2D1_POINT_2F _origin) const
	{
		if (m_pLayout == nullptr || _pTarget == nullptr || _pBrush == nullptr) { return; }
		_pTarget->DrawTextLayout(_origin, m_pLayout.Get(), _pBrush,
			D2D1_DRAW_TEXT_OPTIONS_CLIP);
	}

	std::wstring C_D2D_TEXT_LAYOUT::SurrogateSafeSlice(std::wstring_view _text,
		UINT32 _uStart, UINT32 _uLength)
	{
		std::size_t start = (std::min<std::size_t>)(_uStart, _text.size());
		std::size_t end = (std::min<std::size_t>)(start + static_cast<std::size_t>(_uLength), _text.size());
		if (start > 0 && start < _text.size() && isLowSurrogate(_text[start]) &&
			isHighSurrogate(_text[start - 1]))
		{
			--start;
		}
		if (end > 0 && end < _text.size() && isHighSurrogate(_text[end - 1]) &&
			isLowSurrogate(_text[end]))
		{
			++end;
		}
		return std::wstring(_text.substr(start, end - start));
	}

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

	C_D2D_TEXT_LAYOUT C_D2D_TEXT::CreateLayout(LPCWSTR _pText, IDWriteTextFormat* _pFormat,
		float _fMaxWidth, float _fMaxHeight, UINT32 _uMaxLines)
	{
		C_D2D_TEXT_LAYOUT result;
		if (m_pOwner == nullptr || _pText == nullptr || _pFormat == nullptr) { return result; }
		IDWriteFactory* pDW = m_pOwner->GetDWrite();
		if (pDW == nullptr) { return result; }

		result.m_sText = _pText;
		const float maxWidth = (_fMaxWidth > 0.0f) ? _fMaxWidth : 100000.0f;
		const float maxHeight = (_fMaxHeight > 0.0f) ? _fMaxHeight : 100000.0f;
		HRESULT hr = pDW->CreateTextLayout(result.m_sText.data(),
			static_cast<UINT32>(result.m_sText.size()), _pFormat, maxWidth, maxHeight,
			result.m_pLayout.GetAddressOf());
		if (FAILED(hr)) { return C_D2D_TEXT_LAYOUT{}; }
		result.m_pLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

		if (_uMaxLines > 0)
		{
			const std::vector<DWRITE_LINE_METRICS> lines = result.GetLineMetrics();
			if (lines.size() > _uMaxLines)
			{
				float visibleHeight = 0.0f;
				for (UINT32 i = 0; i < _uMaxLines; ++i) { visibleHeight += lines[i].height; }
				hr = result.m_pLayout->SetMaxHeight((std::min)(maxHeight, visibleHeight));
				if (SUCCEEDED(hr))
				{
					DWRITE_TRIMMING trimming{};
					trimming.granularity = DWRITE_TRIMMING_GRANULARITY_WORD;
					hr = pDW->CreateEllipsisTrimmingSign(_pFormat,
						result.m_pTrimmingSign.GetAddressOf());
					if (SUCCEEDED(hr))
					{
						hr = result.m_pLayout->SetTrimming(&trimming, result.m_pTrimmingSign.Get());
						if (SUCCEEDED(hr)) { result.m_uVisibleLineCap = _uMaxLines; }
					}
				}
			}
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	// IDWriteTextLayout 으로 실제 픽셀 크기 측정. 라벨 배치/히트테스트용.
	//------------------------------------------------------------------------------------------------
	D2D1_SIZE_F C_D2D_TEXT::Measure(LPCWSTR _pText, IDWriteTextFormat* _pFormat, float _fMaxWidth)
	{
		D2D1_SIZE_F size{ 0.0f, 0.0f };
		if (m_pOwner == nullptr || _pText == nullptr || _pFormat == nullptr) { return size; }

		return CreateLayout(_pText, _pFormat, _fMaxWidth).Measure();
	}
}
