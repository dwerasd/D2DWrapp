// D2DText.h: DirectWrite 텍스트 포맷 캐시 + 측정 (한글)
#pragma once

#include "D2DDef.h"
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>


namespace d2d
{
	class C_D2D_DEVICE;

	// IDWriteTextLayout 의 수명과 카드 목록에서 필요한 관측 API를 한 객체로 묶는다.
	class C_D2D_TEXT_LAYOUT
	{
	private:
		Microsoft::WRL::ComPtr<IDWriteTextLayout> m_pLayout;
		Microsoft::WRL::ComPtr<IDWriteInlineObject> m_pTrimmingSign;
		std::wstring m_sText;
		UINT32 m_uVisibleLineCap = 0;

		friend class C_D2D_TEXT;

	public:
		C_D2D_TEXT_LAYOUT() = default;

		bool IsValid() const { return m_pLayout != nullptr; }
		IDWriteTextLayout* Get() const { return m_pLayout.Get(); }

		D2D1_SIZE_F Measure() const;
		std::vector<DWRITE_LINE_METRICS> GetLineMetrics() const;

		// UTF-16 위치 -> DIP 좌표, DIP 좌표 -> UTF-16 위치.
		bool HitTestTextPosition(UINT32 _uPosition, bool _bTrailing,
			D2D1_POINT_2F* _pPoint, DWRITE_HIT_TEST_METRICS* _pMetrics = nullptr) const;
		bool HitTestPoint(float _fX, float _fY, UINT32* _pPosition, bool* _pTrailing,
			bool* _pInside = nullptr, DWRITE_HIT_TEST_METRICS* _pMetrics = nullptr) const;

		void Draw(ID2D1RenderTarget* _pTarget, ID2D1Brush* _pBrush,
			D2D1_POINT_2F _origin = D2D1::Point2F()) const;

		// 요청 범위가 서로게이트 쌍 한가운데에서 시작/끝나면 쌍 전체를 보존한다.
		static std::wstring SurrogateSafeSlice(std::wstring_view _text,
			UINT32 _uStart, UINT32 _uLength);
		std::wstring Slice(UINT32 _uStart, UINT32 _uLength) const
		{
			return SurrogateSafeSlice(m_sText, _uStart, _uLength);
		}
	};

	// 텍스트 포맷은 디바이스 독립(DWriteFactory) → 완전 공유, 로스 무관. 1개 공유.
	// 폰트/크기/굵기별 IDWriteTextFormat 을 캐시한다.
	class C_D2D_TEXT
	{
	private:
		C_D2D_DEVICE* m_pOwner;
		std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDWriteTextFormat>> m_mapFormats;

	public:
		C_D2D_TEXT();
		~C_D2D_TEXT();

		bool Initialize(C_D2D_DEVICE* _pOwner);
		void Shutdown();

		// 폰트/크기/굵기 -> 포맷(캐시). _pFont 예: L"맑은 고딕". 정렬은 호출자가 SetTextAlignment.
		IDWriteTextFormat* GetFormat(LPCWSTR _pFont, float _fSize,
			DWRITE_FONT_WEIGHT _weight = DWRITE_FONT_WEIGHT_NORMAL);

		// word-boundary 우선 wrap + 긴 단어 emergency break. _uMaxLines>0 이면 마지막
		// 표시 줄에 DirectWrite ellipsis trimming 을 적용한다.
		C_D2D_TEXT_LAYOUT CreateLayout(LPCWSTR _pText, IDWriteTextFormat* _pFormat,
			float _fMaxWidth, float _fMaxHeight = 100000.0f, UINT32 _uMaxLines = 0);

		// 텍스트 레이아웃 측정(DIP). _fMaxWidth<=0 이면 무제한.
		D2D1_SIZE_F Measure(LPCWSTR _pText, IDWriteTextFormat* _pFormat, float _fMaxWidth = 0.0f);
	};
}
