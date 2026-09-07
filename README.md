# 3d-engine-test — DirectX 11 자체 엔진

DirectX 11로 직접 만든 3D 게임 엔진과 그 위에서 돌아가는 에디터·게임 클라이언트입니다. 재직 중 개인 과제로 작업했고, 디퍼드 렌더링 파이프라인·GPU 스키닝 인스턴싱·컴퓨트 셰이더 기반 시뮬레이션이 중심입니다.

시연 영상: **[YouTube @jonggu5399](https://www.youtube.com/@jonggu5399)**

---

## ⚠️ 작성 구간 안내

이 저장소는 **직접 작성한 구간**과 **AI 도구로 작성한 구간**이 커밋 히스토리 상에서 나뉩니다.

| 구간 | 범위 | 내용 |
|---|---|---|
| **직접 작성** | 최초 커밋 `e073ddb` (2023-10-22) ~ **`acddc8d` (2024-05-12)** | 엔진 전체, 셰이더 13종, 에디터, 아래 "주요 구현" 항목 전부 |
| AI 보조 | `7879a31` (2026-05-04) ~ 현재 | 컴포넌트 기반 리팩터링, 게임 콘텐츠(타워디펜스), 복셀, 이펙트 추가분 |

**포트폴리오로 봐주실 코드는 `acddc8d` 시점입니다.**

```bash
git clone https://github.com/chimec153/3d-engine-test.git
cd 3d-engine-test
git checkout acddc8d
```

아래 문서의 모든 파일 경로·함수명·줄 번호는 이 커밋 기준입니다.

---

## 주요 구현

`acddc8d` 기준. 셰이더는 전부 직접 작성했습니다.

| 항목 | 요약 | 영상 |
|---|---|---|
| **디퍼드 렌더링 파이프라인** | G-buffer 4장 + 데칼 G-buffer 4장을 조명 패스에서 픽셀 단위로 합성. 깊이에서 뷰 좌표를 복원해 조명·하드웨어 PCF 그림자·높이 포그를 한 번에 계산 | `[영상: ]` |
| **메쉬 스키닝 인스턴싱** | 모든 시퀀스의 키프레임을 하나의 팔레트 버퍼에 굽고, (본 × 인스턴스) 2D 컴퓨트 디스패치로 캐릭터별 본 행렬을 계산. 서로 다른 애니메이션을 재생하는 캐릭터들을 한 번의 `DrawInstanced`로 그림 | `[영상: ]` |
| **스크린 스페이스 데칼** | 깊이 버퍼에서 복원한 뷰 좌표를 역 world-view로 데칼 로컬 공간에 넣어 판정. 별도 G-buffer에 재질을 기록해 데칼도 노멀맵·스페큘러를 갖고 조명을 받음. 인스턴싱·PBR 변형 포함 | `[영상: ]` |
| **GPU 파티클** | 상태를 `RWStructuredBuffer`에 두고 컴퓨트에서 생성·소멸·적분. 정점 버퍼 없이 `DrawInstanced` → 지오메트리 셰이더가 빌보드로 전개 | `[영상: ]` |
| **애니메이션 블렌딩** | 키프레임 보간과 쿼터니언 slerp를 컴퓨트에서 수행. 관절별 블렌드 팔레트로 additive 시퀀스를 부위별 가중치로 혼합. 루트 모션, 노티파이, FABRIK IK | `[영상: ]` |
| **검광 (Trail)** | 칼끝 궤적을 동적 정점 버퍼 스트립으로 생성. 전용 렌더 타깃 → 컴퓨트 블러 → 풀스크린 합성으로 렌더 패스를 하나 추가 | `[영상: ]` |
| **유체 시뮬레이션** | 2D 파동방정식 높이장을 컴퓨트로 풀고 버퍼 3개를 링으로 순환. 정점 셰이더가 높이를 읽어 격자를 변위시키고 중앙차분으로 노멀 생성 | `[영상: ]` |
| **3D 페이퍼 번** | 노이즈 텍스처 기반 디졸브. 타는 경계를 이미시브로 기록해 블룸으로 번지게 함 | `[영상: ]` |
| **천 시뮬레이션** | CPU 매스-스프링(구조·전단·거리 스프링 + 댐퍼 + 바람 + 구 충돌) | `[영상: ]` |
| **스카이박스 / 환경 반사** | 큐브맵 스카이박스와 구면 매핑 환경 반사 | `[영상: ]` |
| **HDR · 블룸 · DOF** | 컴퓨트 휘도 다운스케일 + 눈 순응, 분리형 가우시안 블룸, 거리 기반 DOF | `[영상: ]` |
| **내비게이션 메쉬** | Recast/Detour 통합 — 빌드 파이프라인 배선, 직렬화, dtCrowd 에이전트 래핑, 디버그 메쉬 생성 | `[영상: ]` |
| **에디터** | ImGui 기반. 씬 저장/로드, 지형 브러시, 마우스 피킹, 머티리얼·라이트·포스트프로세스 실시간 조정 | `[영상: ]` |

---

## 구조

아래 구조와 셰이더 목록은 포트폴리오 기준 커밋 `acddc8d` 기준입니다. `main`에는 이후 AI 보조 구간에서 추가된 디렉터리(`Engine/Include/Component`, `.../GameObject`, `.../Voxel`, `Game/`)와 셰이더가 더 있습니다.

```
230301.sln
├─ Engine/Include/          엔진 (DLL)
│   ├─ Bindable/            D3D 리소스 래퍼 + 기능 컴포넌트
│   │                       (Decal, Particle, Fluid, Cloth, PaperBurn,
│   │                        SkyBox, Animation, Terrain, NavMesh, Agent …)
│   ├─ Render/              RenderManager(렌더 패스 오케스트레이션), MRT, RenderInstancing
│   ├─ Shader/              ShaderManager, StructuredBuffer
│   ├─ Animation/           Skeleton, Sequence, JointSocket, Notify
│   ├─ Collision/           OBB SAT, 광선-삼각형, 지형 피킹, 스윕 구
│   ├─ Navigation/Detour/   외부 라이브러리
│   ├─ Core/ Scene/ UI/ Thread/ Resource/ Input/ Sound/
├─ Editor/Include/          ImGui 에디터 + Recast
├─ Client/Include/          게임 클라이언트
└─ */Bin/Resource/Shader/   셰이더 원본 (실행 프로젝트별 사본)
```

### 렌더 패스 순서

`RenderManager::Render()` — `Engine/Include/Render/RenderManager.cpp`

```
RenderOpaque      → G-buffer 4장(t11~t14) + Depth(t10)
RenderDecal       → 데칼 G-buffer 4장(t25~t28)
RenderShadow      → 광원 시점 깊이(D32, t15)
RenderLight       → HDR 타깃(R16G16B16A16)에 조명 가산 누적
RenderSkyBox
RenderAlpha
RenderBlur        → 검광/파티클 전용 타깃 + 컴퓨트 블러 → HDR 타깃 합성
PostProcessing    → 휘도 다운스케일 → 톤매핑 + DOF + 블룸 → 백버퍼
RenderUI
```

### 셰이더

| 파일 | 내용 |
|---|---|
| `anisotropic_microfacet.hlsl` | 메인 셰이더 — G-buffer VS/PS 전 변형(Skin/NoSkin/Inst), 디퍼드 조명 `PS_Multi`, PCF 그림자, 테셀레이션 포인트 라이트 볼륨(HS/DS), 알파 패스 |
| `shared.hlsl` | 공용 cbuffer(b0~b12) · SRV(t0~t41) · UAV(u0~u6) 정의, BRDF·범프·포그·깊이 유틸 |
| `ComputeShader.hlsl` | 애니메이션 본 행렬 (`Sequence` / `SequenceInst`), 유체 (`CS_FLUID`) |
| `Decal.fx` | 스크린 스페이스 데칼 (기본 / 인스턴스 / PBR × 2) |
| `Particle.fx` | GPU 파티클 CS·VS·GS·PS + 블러 CS + 풀스크린 합성 |
| `HDR.fx` | 휘도 다운스케일 CS, 톤매핑, DOF, 블룸 |
| `NormalShader.hlsl`, `Shadow.hlsl`, `PixelShader.hlsl`, `VertexShader.hlsl`, `TextureShader.hlsl`, `UI.fx`, `Debug.hlsl` | 보조 |

셰이더는 `D3DCompileFromFile`로 런타임에 컴파일합니다(`Engine/Include/Bindable/Shader.cpp:44`).

---

## 빌드

- Visual Studio 2022 (플랫폼 툴셋 v143)
- Windows SDK 10.0.22621.0
- x64
- DirectX 11 (Windows SDK 포함)

`230301.sln`을 열고 `Engine` → `Editor` / `Client` 순으로 빌드합니다. 셰이더는 런타임에 컴파일되므로 `Bin/Resource/Shader/` 아래 `.hlsl` / `.fx` 원본이 실행 파일 옆에 있어야 합니다.

---

## 외부 라이브러리

| 라이브러리 | 위치 | 용도 |
|---|---|---|
| Detour | `Engine/Include/Navigation/Detour/` | 경로 탐색, 군중 시뮬레이션 |
| Recast | `Editor/Include/Navigation/Recast/` | 내비게이션 메쉬 생성 |
| Dear ImGui | `Editor/Include/Imgui/` | 에디터 UI |
| FMOD | `Engine/Include/Sound/` | 사운드 |
| DirectXTex | `Engine/Include/Bindable/DirectXTex.h` | 텍스처 로딩 |

일부 알고리즘은 공개 교재를 따랐습니다. HDR 휘도 다운스케일·톤매핑·DOF·블룸과 테셀레이션 포인트 라이트 볼륨은 Jason Zink 외 *Practical Rendering and Computation with Direct3D 11*, 유체 파동방정식 계수와 안정 조건은 Eric Lengyel *Mathematics for 3D Game Programming and Computer Graphics*, 마이크로패싯 BRDF는 표준 문헌을 참조했습니다. GPU 이식(컴퓨트 셰이더화, 구조적 버퍼 설계, 버퍼 순환)과 엔진 리소스 배선·렌더 패스 통합은 직접 작성했습니다.

에셋 출처는 [`CC.txt`](CC.txt)에 있습니다.

---

## 라이선스

개인 학습·포트폴리오 목적의 저장소입니다. 포함된 외부 라이브러리와 에셋은 각자의 라이선스를 따릅니다.
