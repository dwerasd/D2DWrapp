// D2DBrushCache.h: 색 -> SolidColorBrush 캐시 (디바이스 종속, 전 창 공유)
#pragma once

#include "D2DDef.h"
#include <unordered_map>


namespace d2d
{
	class C_D2D_DEVICE;

	// 디바이스당 1개 공유. 브러시는 디바이스 종속이므로 로스 시 OnDeviceLost 로 전부 폐기한다.
	// 같은 디바이스의 리소스 DC 로 생성하면 모든 창(컨텍스트)에서 사용 가능.
	class C_D2D_BRUSH_CACHE
	{
	private:
		C_D2D_DEVICE* m_pOwner;
		std::unordered_map<Color, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> m_mapBrushes;

	public:
		C_D2D_BRUSH_CACHE();
		~C_D2D_BRUSH_CACHE();

		bool Initialize(C_D2D_DEVICE* _pOwner);
		void Shutdown();
		void OnDeviceLost();	// 디바이스 종속 → 전부 폐기(다음 GetBrush 에서 재생성)

		ID2D1SolidColorBrush* GetBrush(Color _argb);	// 없으면 생성
	};
}
