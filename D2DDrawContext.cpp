// D2DDrawContext.cpp
#include "D2DDrawContext.h"
#include "D2DDevice.h"
#include "D2DSwapTarget.h"
#include "D2DBrushCache.h"
#include "D2DText.h"


namespace d2d
{
	C_D2D_DRAW_CONTEXT::C_D2D_DRAW_CONTEXT()
		: m_pDevice(nullptr)
		, m_pDC(nullptr)
		, m_pBrush(nullptr)
		, m_pText(nullptr)
	{
	}

	void C_D2D_DRAW_CONTEXT::Bind(C_D2D_DEVICE* _pDevice, C_D2D_SWAP_TARGET* _pTarget,
		C_D2D_BRUSH_CACHE* _pBrush, C_D2D_TEXT* _pText)
	{
		m_pDevice = _pDevice;
		m_pDC = (_pTarget != nullptr) ? _pTarget->GetDC() : nullptr;
		m_pBrush = _pBrush;
		m_pText = _pText;
	}

	ID2D1SolidColorBrush* C_D2D_DRAW_CONTEXT::brush_(Color _argb)
	{
		return (m_pBrush != nullptr) ? m_pBrush->GetBrush(_argb) : nullptr;
	}

	void C_D2D_DRAW_CONTEXT::Clear(Color _argb)
	{
		if (m_pDC != nullptr) { m_pDC->Clear(ToColorF(_argb)); }
	}

	void C_D2D_DRAW_CONTEXT::DrawLine(float _x0, float _y0, float _x1, float _y1, Color _argb, float _fWidth)
	{
		ID2D1SolidColorBrush* pB = brush_(_argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->DrawLine(D2D1::Point2F(_x0, _y0), D2D1::Point2F(_x1, _y1), pB, _fWidth);
	}

	void C_D2D_DRAW_CONTEXT::FillRect(float _l, float _t, float _r, float _b, Color _argb)
	{
		ID2D1SolidColorBrush* pB = brush_(_argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->FillRectangle(D2D1::RectF(_l, _t, _r, _b), pB);
	}

	void C_D2D_DRAW_CONTEXT::DrawRect(float _l, float _t, float _r, float _b, Color _argb, float _fWidth)
	{
		ID2D1SolidColorBrush* pB = brush_(_argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->DrawRectangle(D2D1::RectF(_l, _t, _r, _b), pB, _fWidth);
	}

	void C_D2D_DRAW_CONTEXT::FillCircle(float _cx, float _cy, float _fRadius, Color _argb)
	{
		ID2D1SolidColorBrush* pB = brush_(_argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->FillEllipse(D2D1::Ellipse(D2D1::Point2F(_cx, _cy), _fRadius, _fRadius), pB);
	}

	void C_D2D_DRAW_CONTEXT::DrawCircle(float _cx, float _cy, float _fRadius, Color _argb, float _fWidth)
	{
		ID2D1SolidColorBrush* pB = brush_(_argb);
		if (m_pDC == nullptr || pB == nullptr) { return; }
		m_pDC->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(_cx, _cy), _fRadius, _fRadius), pB, _fWidth);
	}

	//------------------------------------------------------------------------------------------------
	// 다각형 채우기 — path 지오메트리(팩토리, 디바이스 독립)로 생성 후 FillGeometry.
	//------------------------------------------------------------------------------------------------
	void C_D2D_DRAW_CONTEXT::fillPolygon_(const D2D1_POINT_2F* _pPts, UINT _uCount, Color _argb)
	{
		ID2D1SolidColorBrush* pB = brush_(_argb);
		if (m_pDC == nullptr || m_pDevice == nullptr || pB == nullptr || _pPts == nullptr || _uCount < 3) { return; }

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

	void C_D2D_DRAW_CONTEXT::FillQuad(const D2D1_POINT_2F _pts[4], Color _argb)
	{
		fillPolygon_(_pts, 4, _argb);
	}

	void C_D2D_DRAW_CONTEXT::FillTriangle(const D2D1_POINT_2F _pts[3], Color _argb)
	{
		fillPolygon_(_pts, 3, _argb);
	}

	void C_D2D_DRAW_CONTEXT::DrawText(LPCWSTR _pText, const D2D1_RECT_F& _rc, Color _argb,
		LPCWSTR _pFont, float _fSize)
	{
		ID2D1SolidColorBrush* pB = brush_(_argb);
		if (m_pDC == nullptr || m_pText == nullptr || pB == nullptr || _pText == nullptr) { return; }

		IDWriteTextFormat* pFormat = m_pText->GetFormat(_pFont, _fSize);
		if (pFormat == nullptr) { return; }

		m_pDC->DrawText(_pText, static_cast<UINT32>(::wcslen(_pText)), pFormat, &_rc, pB,
			D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
	}

	void C_D2D_DRAW_CONTEXT::PushClip(const D2D1_RECT_F& _rc)
	{
		if (m_pDC != nullptr) { m_pDC->PushAxisAlignedClip(_rc, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE); }
	}

	void C_D2D_DRAW_CONTEXT::PopClip()
	{
		if (m_pDC != nullptr) { m_pDC->PopAxisAlignedClip(); }
	}
}
