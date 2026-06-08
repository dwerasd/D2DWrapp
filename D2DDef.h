// D2DDef.h: Direct2D 시스템 헤더 및 기본 타입 정의
#pragma once

#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <dwrite.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl/client.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <cstdint>


namespace d2d
{
	// 32비트 ARGB 색 (0xAARRGGBB)
	using Color = uint32_t;

	// ARGB -> D2D1_COLOR_F (정규화 0~1). 알파 포함.
	inline D2D1_COLOR_F ToColorF(Color _argb)
	{
		const float fA = static_cast<float>((_argb >> 24) & 0xFF) / 255.0f;
		const float fR = static_cast<float>((_argb >> 16) & 0xFF) / 255.0f;
		const float fG = static_cast<float>((_argb >> 8) & 0xFF) / 255.0f;
		const float fB = static_cast<float>((_argb) & 0xFF) / 255.0f;
		return D2D1::ColorF(fR, fG, fB, fA);
	}

	// 선 끝/연결 스타일
	enum E_STROKE_CAP : uint8_t
	{
		CAP_FLAT = 0,
		CAP_ROUND,
		CAP_SQUARE,
	};
}
