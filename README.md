# D2DWrapp

Windows Direct2D/DirectWrite 렌더링을 위한 정적 래퍼 라이브러리다. D3D11 기반 공유 디바이스와 DXGI 스왑 체인, 도형·텍스트 그리기, DXGui용 Direct2D 백엔드를 제공한다.

## Public API 범위

- `d2d::C_D2D_DEVICE` — D3D11/Direct2D/DirectWrite 공유 디바이스
- `d2d::C_D2D_SWAP_TARGET` — HWND용 DXGI 스왑 체인과 Direct2D 타깃
- `d2d::C_D2D_DRAW_CONTEXT` — 도형·텍스트 그리기 컨텍스트
- `d2d::C_D2D_BRUSH_CACHE`, `d2d::C_D2D_TEXT` — 브러시와 텍스트 포맷 캐시
- `d2d::C_DRAW_CONTEXT_D2D` — `dxgui::IDrawContext`의 현재 Direct2D 구현

## 요구 사항과 빌드 전제

- Windows SDK의 Direct2D 1.1, DirectWrite, D3D11, DXGI
- 같은 부모 디렉터리의 [DXGui](../DXGui) (`D2DGuiContext.h`가 `DXGui/DxgDrawContext.h` 사용)
- v145 툴셋의 Visual C++ 프로젝트. Debug/Release는 C++20, ReleaseMD는 C++Latest
- Win32/x64 정적 라이브러리 프로젝트

`D2DGuiContext.h`의 형제 `DXGui` include를 해석할 수 있도록 소비 솔루션이나 속성 시트에서 솔루션 루트 또는 부모 디렉터리를 include 경로로 제공해야 한다. ReleaseMD 구성에는 `$(SolutionDir)`와 `$(ProjectDir)..\`가 설정되어 있다.

## 라이선스

이 프로젝트는 MIT License에 따라 배포됩니다.
