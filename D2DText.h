// D2DText.h: DirectWrite 텍스트 포맷 캐시 + 측정 (한글)
#pragma once

#include "D2DDef.h"
#include <unordered_map>
#include <string>


namespace d2d
{
	class C_D2D_DEVICE;

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

		// 텍스트 레이아웃 측정(픽셀). _fMaxWidth<=0 이면 무제한.
		D2D1_SIZE_F Measure(LPCWSTR _pText, IDWriteTextFormat* _pFormat, float _fMaxWidth = 0.0f);
	};
}
