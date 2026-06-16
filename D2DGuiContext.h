// D2DGuiContext.h: dxgui::IDrawContext 의 Direct2D 구현(C_DRAW_CONTEXT_D2D).
// DXGui 위젯/매니저가 본 구현을 통해 D2D 로 렌더. 도형/텍스트는 m_pDC(타겟 DC) 직접,
// 측정은 C_D2D_TEXT(DirectWrite), 입력은 호스트가 매 프레임 주입(폴링 모델).
// D2DWrapp 는 DXGui 추상 헤더만 의존(역방향). 매크로(DrawText->DrawTextW) 일관성 위해
// D2DDef.h(=windows.h) 를 DXGui 헤더보다 먼저 include.
#pragma once

#include "D2DDef.h"
#include <DXGui/DxgDrawContext.h>

#include <vector>
#include <string>


namespace d2d
{
	class C_D2D_DEVICE;
	class C_D2D_SWAP_TARGET;
	class C_D2D_BRUSH_CACHE;
	class C_D2D_TEXT;


	// dxgui::IDrawContext 의 D2D 백엔드. 프레임마다 Bind 후 매니저가 Render.
	class C_DRAW_CONTEXT_D2D : public dxgui::IDrawContext
	{
	private:
		// 등록 폰트 — FontHandle = 인덱스. DrawText/Measure 시 scale 곱해 DWrite 크기 산출.
		struct _FONT_ENT
		{
			std::wstring       sFace;
			float              fSize;	// 1.0 스케일 기준 DWrite em 크기
			DWRITE_FONT_WEIGHT weight;
		};
		std::vector<_FONT_ENT> m_vFonts;

		C_D2D_DEVICE*       m_pDevice;	// 팩토리(path 지오메트리)용
		ID2D1DeviceContext* m_pDC;		// 현재 프레임 타겟(비소유)
		C_D2D_BRUSH_CACHE*  m_pBrush;
		C_D2D_TEXT*         m_pText;

		// ── 입력 상태(폴링) — 호스트 WndProc 가 갱신 ──
		dxgui::_DXG_POINT m_Mouse;
		bool m_bDown[3];		// 이번 프레임 버튼 down
		bool m_bPrevDown[3];	// 직전 프레임 down (clicked/released 산출)
		bool m_bKey[256];		// 이번 프레임 VK down
		std::wstring m_sTextInput;	// 이번 프레임 텍스트 입력(IME 포함)
		float m_fWheel;			// 이번 프레임 휠 누적(노치)

		ID2D1SolidColorBrush* brush_(uint32_t _argb);
		IDWriteTextFormat*    fmt_(dxgui::FontHandle _h, float _fScale);
		void fillPolygon_(const D2D1_POINT_2F* _pPts, UINT _uCount, uint32_t _argb);

	public:
		C_DRAW_CONTEXT_D2D();

		// 프레임 바인드(타겟 DC 갱신). _pTarget 은 BeginDraw 상태여야 한다.
		void Bind(C_D2D_DEVICE* _pDevice, C_D2D_SWAP_TARGET* _pTarget,
			C_D2D_BRUSH_CACHE* _pBrush, C_D2D_TEXT* _pText);

		// 타겟 클리어(프레임 시작). IDrawContext 위젯 프리미티브 아님 — 호스트 프레임 op.
		void Clear(uint32_t _argb);

		// ── 입력 주입(호스트) ──
		void SetMousePos(float _x, float _y) { m_Mouse.x = _x; m_Mouse.y = _y; }
		void SetMouseButton(dxgui::E_DXG_MOUSE_BUTTON _btn, bool _bDown);
		void SetKey(int _nVK, bool _bDown);
		void PushTextInput(const wchar_t* _pText);	// WM_CHAR/IME 결과 누적
		void AddWheel(float _fNotches) { m_fWheel += _fNotches; }	// WM_MOUSEWHEEL(+위/-아래)
		// 프레임 경계 — 매니저 Render 직후 호출: prevDown=down, 키/텍스트 큐 클리어.
		void NewFrame();

		// ── dxgui::IDrawContext ──
		void BeginFrame() override {}
		void EndFrame() override {}

		dxgui::FontHandle RegisterFont(const wchar_t* _pFace, uint32_t _uPxHeight, bool _bBold) override;
		dxgui::_DXG_SIZE  MeasureText(dxgui::FontHandle _hFont, const wchar_t* _pText, float _fScale) override;
		float             GetFontHeight(dxgui::FontHandle _hFont, float _fScale) override;
		void DrawText(dxgui::FontHandle _hFont, dxgui::_DXG_POINT _pos,
			const wchar_t* _pText, dxgui::_DXG_COLOR _color, float _fScale) override;

		void FillRect(dxgui::_DXG_RECT _rect, dxgui::_DXG_COLOR _color) override;
		void DrawRectOutline(dxgui::_DXG_RECT _rect, dxgui::_DXG_COLOR _color, float _fThickness) override;
		void DrawLine(dxgui::_DXG_POINT _a, dxgui::_DXG_POINT _b, dxgui::_DXG_COLOR _color, float _fThickness) override;
		void PushClipRect(dxgui::_DXG_RECT _rect) override;
		void PopClipRect() override;

		void FillQuad(const dxgui::_DXG_POINT _pts[4], dxgui::_DXG_COLOR _color) override;
		void FillTriangle(const dxgui::_DXG_POINT _pts[3], dxgui::_DXG_COLOR _color) override;
		void FillCircle(dxgui::_DXG_POINT _c, float _fRadius, dxgui::_DXG_COLOR _color) override;
		void DrawCircle(dxgui::_DXG_POINT _c, float _fRadius, dxgui::_DXG_COLOR _color, float _fThickness) override;

		dxgui::_DXG_POINT GetMousePos() const override { return m_Mouse; }
		bool IsMouseHovered(dxgui::_DXG_RECT _rect) const override { return _rect.Contains(m_Mouse.x, m_Mouse.y); }
		bool IsMouseClicked(dxgui::E_DXG_MOUSE_BUTTON _btn) const override;
		bool IsMouseDown(dxgui::E_DXG_MOUSE_BUTTON _btn) const override;
		bool IsMouseReleased(dxgui::E_DXG_MOUSE_BUTTON _btn) const override;
		bool IsKeyPressed(int _nVK) const override;
		const wchar_t* PollTextInput() const override
		{
			return m_sTextInput.empty() ? nullptr : m_sTextInput.c_str();
		}
		float GetWheelDelta() const override { return m_fWheel; }
	};
}
