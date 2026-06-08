// D2DDrawContext.h: 고수준 2D 프리미티브 파사드 (차트 위젯이 그리는 표면)
#pragma once

#include "D2DDef.h"


namespace d2d
{
	class C_D2D_DEVICE;
	class C_D2D_SWAP_TARGET;
	class C_D2D_BRUSH_CACHE;
	class C_D2D_TEXT;

	// 한 프레임 동안 대상 SwapTarget 의 DC 에 그린다. 브러시/텍스트는 공유 캐시 참조.
	// 프레임 흐름: target.BeginDraw(); ctx.Bind(...); ctx.Clear(); ctx.DrawXxx(...); target.EndDraw();
	class C_D2D_DRAW_CONTEXT
	{
	private:
		C_D2D_DEVICE*       m_pDevice;	// 팩토리(지오메트리)용
		ID2D1DeviceContext* m_pDC;		// 현재 프레임 타겟(비소유)
		C_D2D_BRUSH_CACHE*  m_pBrush;
		C_D2D_TEXT*         m_pText;

		ID2D1SolidColorBrush* brush_(Color _argb);
		void fillPolygon_(const D2D1_POINT_2F* _pPts, UINT _uCount, Color _argb);

	public:
		C_D2D_DRAW_CONTEXT();

		// 프레임 시작 시 바인드(타겟 DC 갱신). target 은 BeginDraw 된 상태여야 한다.
		void Bind(C_D2D_DEVICE* _pDevice, C_D2D_SWAP_TARGET* _pTarget,
			C_D2D_BRUSH_CACHE* _pBrush, C_D2D_TEXT* _pText);

		void Clear(Color _argb);

		void DrawLine(float _x0, float _y0, float _x1, float _y1, Color _argb, float _fWidth = 1.0f);
		void FillRect(float _l, float _t, float _r, float _b, Color _argb);
		void DrawRect(float _l, float _t, float _r, float _b, Color _argb, float _fWidth = 1.0f);
		void FillCircle(float _cx, float _cy, float _fRadius, Color _argb);		// 마커
		void DrawCircle(float _cx, float _cy, float _fRadius, Color _argb, float _fWidth = 1.0f);
		void FillQuad(const D2D1_POINT_2F _pts[4], Color _argb);					// 밴드/영역
		void FillTriangle(const D2D1_POINT_2F _pts[3], Color _argb);				// 화살표/마커

		// 폰트/크기 지정 텍스트. 좌상단 정렬. (정밀 정렬/측정은 C_D2D_TEXT 사용)
		void DrawText(LPCWSTR _pText, const D2D1_RECT_F& _rc, Color _argb,
			LPCWSTR _pFont, float _fSize);

		void PushClip(const D2D1_RECT_F& _rc);
		void PopClip();
	};
}
