# D2DWrapp

Windows Direct2D/DirectWrite 렌더링을 위한 정적 래퍼 라이브러리다. D3D11 기반 공유 디바이스와 DXGI 스왑 체인, 도형·텍스트 그리기, DXGui용 Direct2D 백엔드를 제공한다.

## Public API 범위

- `d2d::C_D2D_DEVICE` — D3D11/Direct2D/DirectWrite 공유 디바이스
- `d2d::C_D2D_SWAP_TARGET` — HWND용 DXGI 스왑 체인과 Direct2D 타깃
- `d2d::C_D2D_DRAW_CONTEXT` — 도형·텍스트 그리기 컨텍스트
- `d2d::C_D2D_BRUSH_CACHE`, `d2d::C_D2D_TEXT` — 브러시와 텍스트 포맷 캐시
- `d2d::C_D2D_TEXT_LAYOUT` — `IDWriteTextLayout` 소유, 줄 메트릭, 마지막 줄
  말줄임, UTF-16 양방향 hit-test, 서로게이트 안전 slice와 직접 draw
- `d2d::C_DRAW_CONTEXT_D2D` — `dxgui::IDrawContext`의 현재 Direct2D 구현

## 요구 사항과 빌드 전제

- Windows SDK의 Direct2D 1.1, DirectWrite, D3D11, DXGI
- 같은 부모 디렉터리의 [DXGui](../DXGui) (`D2DGuiContext.h`가 `DXGui/DxgDrawContext.h` 사용)
- v145 툴셋의 Visual C++ 프로젝트. Debug/Release는 C++20, ReleaseMD는 C++Latest
- Win32/x64 정적 라이브러리 프로젝트

`D2DGuiContext.h`의 형제 `DXGui` include를 해석할 수 있도록 소비 솔루션이나 속성 시트에서 솔루션 루트 또는 부모 디렉터리를 include 경로로 제공해야 한다. `ReleaseMD|Win32`, `ReleaseMD|x64`, `Release|x64` 세 구성에는 `$(SolutionDir)`와 `$(ProjectDir)..\`가 설정되어 있으나, Debug 전 구성과 `Release|Win32`에는 없어 그 구성으로 빌드하려면 소비 측이 별도로 include 경로를 제공해야 한다.

`C_D2D_TEXT::CreateLayout`은 DirectWrite의 word-boundary 우선 wrap과 긴 단어
emergency break를 사용한다. 최대 표시 줄 수를 주면 실제 줄 높이로 레이아웃 높이를
제한하고 마지막 표시 줄에 ellipsis trimming을 적용한다. 기존 `GetFormat`과
`Measure` 진입점은 유지되며 `Measure`도 같은 레이아웃 경로를 소비한다.

## 현재 상태

- 이 저장소에는 `.sln`이 없다. `D2DWrapp.vcxproj`는 부모 솔루션에 프로젝트로
  포함되어 빌드되는 것을 전제한다.
- 실빌드로 검증된 구성은 `ReleaseMD|Win32`/`ReleaseMD|x64`뿐이다(커밋
  `d33bb60`: `ReleaseMD|Win32`을 `ReleaseMD|x64`와 동일 계약으로 맞추며
  `PrecompiledHeader`를 `NotUsing`으로 전환, 사유는 소스 6개 중 5개가
  `pch.h`를 include하지 않아 `Use` 상태에서 C1010이 발생했기 때문).
- `Debug|Win32`, `Debug|x64`, `Release|Win32`, `Release|x64` 네 구성은 여전히
  `PrecompiledHeader=Use`다. `D2DGuiContext.cpp`만 프로젝트 항목 단위로
  `NotUsing`이 지정되어 있고, `D2DDevice.cpp`/`D2DSwapTarget.cpp`/
  `D2DBrushCache.cpp`/`D2DText.cpp`/`D2DDrawContext.cpp`는 `pch.h`를
  include하지 않으므로 이 네 구성으로 빌드하면 C1010이 발생할 수 있다(실측
  빌드 미검증).
- `D2DWrapp.cpp`는 Visual Studio 템플릿이 생성한 빈 함수(`fnD2DWrapp`) 하나만
  담긴 스텁이며 실제 기능과 무관하다. 라이브러리 기능은 나머지 `D2D*.h/.cpp`가
  전담한다.

## 라이선스

이 프로젝트는 MIT License에 따라 배포됩니다.
