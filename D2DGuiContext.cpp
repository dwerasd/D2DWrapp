// D2DGuiContext.cpp
#include "D2DGuiContext.h"
#include "D2DDevice.h"
#include "D2DSwapTarget.h"
#include "D2DBrushCache.h"
#include "D2DText.h"

#include <cwchar>


namespace d2d
{
	C_DRAW_CONTEXT_D2D::C_DRAW_CONTEXT_D2D()
		: m_pDevice(nullptr)
		, m_pDC(nullptr)
		, m_pBrush(nullptr)
		, m_pText(nullptr)
	{
		for (int i = 0; i < 3; ++i) { m_bDown[i] = false; m_bPrevDown[i] = false; }
		for (int i = 0; i < 256; ++i) { m_bKey[i] = false; }
	}

	void C_DRAW_CONTEXT_D2D::Bind(C_D2D_DEVICE* _pDevice, C_D2D_SWAP_TARGET* _pTarget,
		C_D2D_BRUSH_CACHE* _pBrush, C_D2D_TEXT* _pText)
	{
		m_pDevice = _pDevice;
		m_pDC     = (_pTarget != nullptr) ? _pTarget->GetDC() : nullptr;
		m_pBrush  = _pBrush;
		m_pText   = _pText;
	}

	//------------------------------------------------------------------------------------------------
	// 입력 주입 / 프레임 경계
	//------------------------------------------------------------------------------------------------
	void C_DRAW_CONTEXT_D2D::SetMouseButton(dxgui::E_DXG_MOUSE_BUTTON _btn, bool _bDown)
	{
		const int i = static_cast<int>(_btn);
		if (i >= 0 && i < 3) { m_bDown[i] = _bDown; }
	}

	void C_DRAW_CONTEXT_D2D::SetKey(int _nVK, bool _bDown)
	{
		if (_nVK >= 0 && _nVK < 256) { m_bKey[_nVK] = _bDown; }
	}

	void C_DRAW_CONTEXT_D2D::PushTextInput(const wchar_t* _pText)
	{
		if (_pText != nullptr) { m_sTextInput += _pText; }
	}

	void C_DRAW_CONTEXT_D2D::NewFrame()
	{
		for (int i = 0; i < 3; ++i) { m_bPrevDown[i] = m_bDown[i]; }
		for (int i = 0; i < 256; ++i) { m_bKey[i] = false; }
		m_sTextInput.clear();
	}

	//------------------------------------------------------------------------------------------------
	// 내부 헬퍼
	//------------------------------------------------------------------------------------------------
	ID2D1SolidColorBrush* C_DRAW_CONTEXT_D2D::brush_(uint32_t _argb)
	{
		return (m_pBrush != nullptr) ? m_pBrush->GetBrush(_argb) : nullptr;
	}

	IDWriteTextFormat* C_DRAW_CONTEXT_D2D::fmt_(dxgui::FontHandle _h, float _fScale)
	{
		if (m_pText == nullptr || _h < 0
			|| _h >= static_cast<dxgui::FontHandle>(m_vFonts.size()))
		{
			return nullptr;
		}
		const _FONT_ENT& e = m_vFonts[static_cast<size_t>(_h)];
		const float fSize = e.fSize * (_fScale > 0.0f ? _fScale : 1.0f);
		return m_pText->GetFormat(e.sFace.c_str(), fSize, e.weight);
	}

	void C_DRAW_CONTEXT_D2D::fillPolygon_(const D2D1_POINT_2F* _pPts, UINT _uCount, uint32_t _argb)
	{
		ID2D1SolidColorBrush* pB = brush_(_argb);
		if (m_pDC == nullptr || m_pDevice == nullptr || pB == nullptr
			|| _pPts == nullptr || _uCount < 3)
		{
			return;
		}
		ID2D1Factory1* pFactory = m_pDevice->GetFactory();
		if (pFactory == nullptr) { return; }

		Microsoft::WRL::ComPtr<ID2D1PathGeometry> pGeo;
		if (FAILED(pFactory->CreatePathGeometry(pGeo.GetAddressOf()))) { return; }
		Microsoft::WRL::ComPtr<ID2D1GeometrySink> pSink;
		if (FAILED(pGeo->Open(pSink.GetAddressOf()))) { return; }
		pSink->BeginFigure(_pPts[0], D2D1_FIGURE_BEGIN_FILLED);
		pSink->AddLines(_pPts + 1, _uCount - 1);
		pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
		pSink->Close();
		m_pDC->FillGeometry(pGeo.Get(), pB);
	}

	//------------------------------------------------------------------------------------------------
	// 폰트 / 텍스트
	//------------------------------------------------------------------------------------------------
	dxgui::FontHandle C_DRAW_CONTEXT_D2D::RegisterFont(const wchar_t* _pFace, uint32_t _uPxHeight, bool _bBold)
	{
		_FONT_ENT e;
		e.sFace  = (_pFace != nullptr) ? _pFace : L"맑은 고딕";
		e.fSize  = static_cast<float>(_uPxHeight);
		e.weight = _bBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
		m_vFonts.push_back(std::move(e));
		return static_cast<dxgui::FontHandle>(m_vFonts.size() - 1);
	}

	dxgui::_DXG_SIZE C_DRAW_CONTEXT_D2D::MeasureText(dxgui::FontHandle _hFont, const wchar_t* _pText, float _fScale)
	{
		IDWriteTextFormat* pFmt = fmt_(_hFont, _fScale);
		if (pFmt == nullptr || _pText == nullptr) { return dxgui::_DXG_SIZE(0.0f, 0.0f); }
		const D2D1_SIZE_F sz = m_pText->Measure(_pText, pFmt);
		return dxgui::_DXG_SIZE(sz.width, sz.height);
	}

	float C_DRAW_CONTEXT_D2D::GetFontHeight(dxgui::FontHandle _hFont, float _fScale)
	{
		IDWriteTextFormat* pFmt = fmt_(_hFont, _fScale);
		if (pFmt == nullptr) { return 0.0f; }
		// 한 줄 높이 — 대표 글리프 측정(상/하 디센더 포함).
		const D2D1_SIZE_F sz = m_pText->Measure(L"Ag", pFmt);
		return sz.height;
	}

	void C_DRAW_CONTEXT_D2D::DrawText(dxgui::FontHandle _hFont, dxgui::_DXG_POINT _pos,
		const wchar_t* _pText, dxgui::_DXG_COLOR _color, float _fScale)
	{
		ID2D1SolidColorBrush* pB = brush_(_color.argb);
		IDWriteTextFormat* pFmt = fmt_(_hFont, _fScale);
		if (m_pDC == nullptr || pB == nullptr || pFmt == nullptr || _pText == nullptr) { return; }
		// 좌상단(_pos) 기준 — 넉넉한 레이아웃 박스, 정렬은 포맷 기본(leading/near).
		const D2D1_RECT_F rc = D2D1::RectF(_pos.x, _pos.y, _pos.x + 100000.0f, _pos.y + 100000.0f);
		m_pDC->DrawText(_pText, static_cast<UINT32>(::wcslen(_pText)), pFmt, &rc, pB,
			D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
	}

	//------------------------------------------------------------------------------------------------
	// 도형
	//------------------------------------------------------------------------------------------------
	void C_DRAW_CONTEXT_D2D::FillRect(dxgui::_DXG_RECT _rect, dxgui::_DXG_COLOR _color)
	{
		ID2D1SolidColorBrush* pB = brush_(_color.argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->FillRectangle(
			D2D1::RectF(_rect.x, _rect.y, _rect.x + _rect.w, _rect.y + _rect.h), pB);
	}

	void C_DRAW_CONTEXT_D2D::DrawRectOutline(dxgui::_DXG_RECT _rect, dxgui::_DXG_COLOR _color, float _fThickness)
	{
		ID2D1SolidColorBrush* pB = brush_(_color.argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->DrawRectangle(
			D2D1::RectF(_rect.x, _rect.y, _rect.x + _rect.w, _rect.y + _rect.h), pB, _fThickness);
	}

	void C_DRAW_CONTEXT_D2D::DrawLine(dxgui::_DXG_POINT _a, dxgui::_DXG_POINT _b, dxgui::_DXG_COLOR _color, float _fThickness)
	{
		ID2D1SolidColorBrush* pB = brush_(_color.argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->DrawLine(D2D1::Point2F(_a.x, _a.y), D2D1::Point2F(_b.x, _b.y), pB, _fThickness);
	}

	void C_DRAW_CONTEXT_D2D::PushClipRect(dxgui::_DXG_RECT _rect)
	{
		if (m_pDC != nullptr)
		{
			m_pDC->PushAxisAlignedClip(
				D2D1::RectF(_rect.x, _rect.y, _rect.x + _rect.w, _rect.y + _rect.h),
				D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		}
	}

	void C_DRAW_CONTEXT_D2D::PopClipRect()
	{
		if (m_pDC != nullptr) { m_pDC->PopAxisAlignedClip(); }
	}

	void C_DRAW_CONTEXT_D2D::FillQuad(const dxgui::_DXG_POINT _pts[4], dxgui::_DXG_COLOR _color)
	{
		const D2D1_POINT_2F p[4] = {
			D2D1::Point2F(_pts[0].x, _pts[0].y), D2D1::Point2F(_pts[1].x, _pts[1].y),
			D2D1::Point2F(_pts[2].x, _pts[2].y), D2D1::Point2F(_pts[3].x, _pts[3].y),
		};
		fillPolygon_(p, 4, _color.argb);
	}

	void C_DRAW_CONTEXT_D2D::FillTriangle(const dxgui::_DXG_POINT _pts[3], dxgui::_DXG_COLOR _color)
	{
		const D2D1_POINT_2F p[3] = {
			D2D1::Point2F(_pts[0].x, _pts[0].y), D2D1::Point2F(_pts[1].x, _pts[1].y),
			D2D1::Point2F(_pts[2].x, _pts[2].y),
		};
		fillPolygon_(p, 3, _color.argb);
	}

	void C_DRAW_CONTEXT_D2D::FillCircle(dxgui::_DXG_POINT _c, float _fRadius, dxgui::_DXG_COLOR _color)
	{
		ID2D1SolidColorBrush* pB = brush_(_color.argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->FillEllipse(D2D1::Ellipse(D2D1::Point2F(_c.x, _c.y), _fRadius, _fRadius), pB);
	}

	void C_DRAW_CONTEXT_D2D::DrawCircle(dxgui::_DXG_POINT _c, float _fRadius, dxgui::_DXG_COLOR _color, float _fThickness)
	{
		ID2D1SolidColorBrush* pB = brush_(_color.argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(_c.x, _c.y), _fRadius, _fRadius), pB, _fThickness);
	}

	//------------------------------------------------------------------------------------------------
	// 입력 조회
	//------------------------------------------------------------------------------------------------
	bool C_DRAW_CONTEXT_D2D::IsMouseClicked(dxgui::E_DXG_MOUSE_BUTTON _btn) const
	{
		const int i = static_cast<int>(_btn);
		return (i >= 0 && i < 3) && !m_bPrevDown[i] && m_bDown[i];
	}

	bool C_DRAW_CONTEXT_D2D::IsMouseDown(dxgui::E_DXG_MOUSE_BUTTON _btn) const
	{
		const int i = static_cast<int>(_btn);
		return (i >= 0 && i < 3) && m_bDown[i];
	}

	bool C_DRAW_CONTEXT_D2D::IsMouseReleased(dxgui::E_DXG_MOUSE_BUTTON _btn) const
	{
		const int i = static_cast<int>(_btn);
		return (i >= 0 && i < 3) && m_bPrevDown[i] && !m_bDown[i];
	}

	bool C_DRAW_CONTEXT_D2D::IsKeyPressed(int _nVK) const
	{
		return (_nVK >= 0 && _nVK < 256) && m_bKey[_nVK];
	}
}
