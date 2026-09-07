# 개인 R&D 포트폴리오 — DirectX 11 자체 엔진

**전종구** | C++ 클라이언트 프로그래머 (IMC게임즈, 2021.08~ / 그라나도 에스파다 · DirectX 9 자체 엔진)

**저장소** https://github.com/chimec153/3d-engine-test

**영상** https://www.youtube.com/@jonggu5399

> **코드 인용 기준** — 이 문서의 모든 파일 경로·함수명·줄 번호는 커밋 `acddc8d` (2024-05-12) 기준입니다. 저장소 최신 상태(main)와 인용 위치가 다를 수 있습니다.
>
> ```
> git checkout acddc8d
> ```

---

## 목차

| # | 항목 | 한 줄 요약 |
|---|---|---|
| 0 | [엔진 기반 — 디퍼드 렌더링 파이프라인](#0-엔진-기반--디퍼드-렌더링-파이프라인) | G-buffer 4장 + 데칼 G-buffer 4장을 조명 패스에서 합성하는 디퍼드 파이프라인. 아래 항목들이 올라가 있는 토대 |
| 1 | [메쉬 스키닝 인스턴싱](#1-메쉬-스키닝-인스턴싱) | 모든 시퀀스의 키프레임을 하나의 팔레트 버퍼에 굽고, (본 × 인스턴스) 2D 컴퓨트 디스패치로 캐릭터별 본 행렬을 계산해 서로 다른 애니메이션을 한 번의 `DrawInstanced`로 그림 |
| 2 | [스크린 스페이스 데칼](#2-스크린-스페이스-데칼) | 깊이 버퍼에서 뷰 좌표를 복원하고 역 world-view로 데칼 로컬 공간에 넣어 단위 박스 내부만 남긴 뒤, 별도 G-buffer에 기록해 조명 패스에서 합성 |
| 3 | [GPU 파티클](#3-gpu-파티클) | 파티클 상태를 `RWStructuredBuffer`에 두고 컴퓨트에서 생성·소멸·적분, 정점 버퍼 없이 `DrawInstanced` → 지오메트리 셰이더가 빌보드 사각형으로 전개 |
| 4 | [애니메이션 블렌딩 (GPU 본 행렬 계산)](#4-애니메이션-블렌딩-gpu-본-행렬-계산) | 키프레임 보간과 쿼터니언 slerp를 컴퓨트에서 수행하고, 관절별 블렌드 팔레트로 additive 시퀀스를 부위별 가중치로 섞음 |
| 5 | [검광 (Trail)](#5-검광-trail) | 칼끝 궤적을 동적 정점 버퍼 스트립으로 만들고, 전용 렌더 타깃 → 컴퓨트 블러 → 풀스크린 합성이라는 렌더 패스를 하나 추가 |
| 6 | [유체 시뮬레이션](#6-유체-시뮬레이션) | 2D 파동방정식 높이장을 컴퓨트로 풀고 버퍼 3개를 링으로 돌리며, 정점 셰이더가 높이를 읽어 격자를 변위시키고 중앙차분으로 노멀을 만듦 |
| A | [부록 — 외부 코드 및 교과서 유래 표기](#부록--외부-코드-및-교과서-유래-표기) | 어느 부분이 직접 작성이고 어느 부분이 라이브러리·교과서 유래인지 |

---

## 0. 엔진 기반 — 디퍼드 렌더링 파이프라인


### 개요

아래 항목들이 개별 이펙트로 따로 존재하는 게 아니라 하나의 디퍼드 렌더링 파이프라인 위에 올라가 있어서, 그 토대를 먼저 정리합니다. 불투명 패스가 G-buffer 4장을 채우고, 데칼 패스가 별도의 G-buffer 4장을 채우고, 조명 패스가 두 G-buffer를 픽셀 단위로 합성하면서 깊이에서 뷰 좌표를 복원해 조명·그림자·포그를 한 번에 계산합니다. 렌더 패스 순서와 리소스 수명은 `RenderManager` 한 곳에서 통제합니다.

### 파이프라인 구성

`RenderManager::Render()` (`RenderManager.cpp:570`)의 패스 순서입니다.

```
RenderOpaque      → G-buffer(MRT 4장, t11~t14) + Depth(t10)
RenderDecal       → Decal G-buffer(MRT 4장, t25~t28)
RenderShadow      → 광원 시점 깊이 버퍼(D32, t15)
RenderLight       → HDR 타깃(R16G16B16A16)에 조명 누적 (가산 블렌드)
RenderSkyBox
RenderAlpha
RenderBlur        → 검광/파티클용 별도 타깃 + 컴퓨트 블러 → HDR 타깃에 합성
PostProcessing    → 휘도 다운스케일 → 톤매핑 + DOF + 블룸 → 백버퍼
RenderUI
```

G-buffer는 `RenderManager::Init()` (`:231`)에서 `DXGI_FORMAT_R8G8B8A8_UNORM` 4장으로 만들고 SRV 시작 슬롯을 11로 잡습니다. 데칼 G-buffer는 같은 포맷 4장에 시작 슬롯 25입니다. 채널 배치는 `value0`=알베도+러프니스x, `value1`=인코딩 노멀+러프니스y, `value2`=G항+메탈릭 비율, `value3`=스페큘러 색입니다.

**깊이에서 뷰 좌표 복원.** 조명 패스는 정점 버퍼 없이 `Draw(4, 0)`으로 풀스크린 스트립을 그립니다(`RenderLight`, `:713`). `VS_Multi`가 `SV_VertexID`로 화면 사각형을 만들고, `PS_Multi`(`anisotropic_microfacet.hlsl:461`)가 깊이를 읽어 뷰 공간 위치를 복원합니다. 투영 행렬의 역수 항들을 `PERSPECTIVEBUFFER`(b3)에 미리 담아 보내므로 셰이더에서는 나눗셈 한 번으로 끝납니다.

```hlsl
// shared.hlsl:527
float ConvertZToLinearDepth(float depth)
{
    float linearDepth = g_vProjectValues.w / (g_vProjectValues.z - depth);
    return linearDepth;
}
// PS_Multi
viewPos.z  = ConvertZToLinearDepth(depth);
viewPos.xy = pos * viewPos.z * g_vProjectValues.xy;
```

**데칼 합성.** 조명 패스는 두 G-buffer를 데칼 알파로 보간해서 최종 재질을 만듭니다. 데칼이 알베도·노멀·러프니스·스페큘러를 각각 독립적으로 덮어씁니다.

```hlsl
float3 albedo = value0.xyz * (1 - decal0.w) + decal0.xyz * decal0.w;
float3 normal = normalize(((value1.xyz * (1 - decal1.w) + decal1.xyz * decal1.w) - 0.5) * 2);
```

**그림자.** 뷰 공간 위치에 `g_matCameraViewToLightClip`을 곱해 광원 클립 공간으로 보내고, `SamplerComparisonState`(s3) + `SampleCmp`로 하드웨어 PCF를 씁니다. 카메라 뷰 → 광원 클립 행렬은 CPU에서 `invView * lightViewProject`로 미리 합성해 보냅니다(`RenderLight`, `:733`).

**조명 모델.** 이방성 마이크로패싯 BRDF입니다. 분포항은 탄젠트 방향 성분 `TDotP`를 받는 이방성 Beckmann(`shared.hlsl:587`), 기하 감쇠는 Cook-Torrance 표준식(`:595`), 프레넬은 굴절률 기반(`float3` 오버로드 `:569`)입니다. 스페큘러는 구면 매핑(`SphereMapping`, `:545`)으로 환경 텍스처를 참조합니다.

### 막혔던 지점과 해결

`PS_Multi` 안에 정식 Cook-Torrance 형태(`kd * lambert + cookTorrance`)를 계산하는 블록이 통째로 주석 처리된 채 남아 있습니다(`anisotropic_microfacet.hlsl:539~561`). 에너지 보존형 BRDF로 정리하려다 실패하고, `materialFraction`으로 디퓨즈/스페큘러를 선형 보간하는 현재 식으로 돌아온 흔적입니다. 지금 쓰는 식은 물리적으로 정확하지 않습니다.

### 한계

- **포인트 라이트 볼륨이 연결되어 있지 않습니다.** 테셀레이션으로 반구 두 개를 만들어 광원 볼륨을 그리는 셰이더(`VS_PointLight` / `HS_PointLight` / `DS_PointLight`, `anisotropic_microfacet.hlsl:574~642`)를 작성했고 `RenderManager::Init()`에서 로드까지 하지만, `RenderLight`는 모든 광원을 풀스크린 쿼드로 처리합니다(`Draw(4, 0)`). 즉 광원 개수만큼 화면 전체 픽셀에 조명 셰이더가 돕니다. 볼륨 경로를 실제로 물리는 작업이 남았습니다.
- G-buffer가 RGBA8 4장이라 노멀을 8비트로 인코딩합니다. 상용 엔진은 보통 노멀을 옥타헤드럴 인코딩으로 2채널에 넣고 16비트를 씁니다.
- 그림자는 광원 하나(디렉셔널)만, 캐스케이드 없이 단일 맵입니다.
- 조명 패스가 가산 블렌드로 광원마다 한 번씩 풀스크린을 도는 구조라 타일드/클러스터드 방식 대비 대역폭이 불리합니다.

### 참조 위치

| 대상 | 위치 |
|---|---|
| 패스 오케스트레이션 | `Engine/Include/Render/RenderManager.cpp` — `Render()` `:570`, `Init()` `:229` |
| 조명 패스 | `RenderLight()` `:713` / `RenderShadow()` `:796` |
| 렌더 타깃 래퍼 | `Engine/Include/Render/MRT.cpp` |
| 조명 셰이더 | `Client/Bin/Resource/Shader/anisotropic_microfacet.hlsl` — `VS_Multi` `:449`, `PS_Multi` `:461` |
| 공용 정의 | `Client/Bin/Resource/Shader/shared.hlsl` — cbuffer b0~b12, SRV t0~t41, UAV u0~u6 |

---

## 1. 메쉬 스키닝 인스턴싱

**영상** https://www.youtube.com/watch?v=H4899uhrnqI

### 개요

같은 메쉬를 쓰는 스키닝 캐릭터 여러 마리를, 각자 다른 애니메이션을 재생하는 상태 그대로 한 번의 `DrawInstanced`로 그립니다. 인스턴싱을 쓰면 보통 애니메이션이 걸림돌이 되는데(캐릭터마다 본 행렬 팔레트가 달라 상수 버퍼를 매번 갈아끼워야 함), 모든 시퀀스의 키프레임을 하나의 구조적 버퍼에 미리 굽고 인스턴스별 재생 상태만 넘겨서 GPU가 인스턴스 수만큼의 본 행렬을 한 번에 계산하게 하는 방식으로 풀었습니다.

### 기법 설명

스키닝은 정점마다 최대 4개의 본 인덱스와 가중치를 갖고, `Σ wᵢ · (Mᵢ · v)` 로 정점을 변형합니다. 여기서 `Mᵢ = invBindPose(i) · currentPose(i)` 입니다. 단일 캐릭터라면 `Mᵢ` 배열(본 행렬 팔레트)을 상수 버퍼에 올리면 되지만, 인스턴싱에서는 인스턴스마다 팔레트가 다릅니다.

해법은 팔레트를 **인스턴스 축으로 이어붙인 하나의 큰 버퍼**로 만들고, 정점 셰이더에서 `SV_InstanceID`로 자기 구간을 찾아가게 하는 것입니다.

```
g_vecBones[ blendIndex + maxJoint * instanceID ]
```

그리고 그 큰 버퍼를 채우는 계산 자체를 컴퓨트 셰이더에 (본 × 인스턴스) 2차원 디스패치로 넘깁니다. x축이 관절, y축이 인스턴스입니다.

### 구현

**(1) 애니메이션 팔레트 굽기 — `RenderInstancing::CreateBoneBuffer()` (`RenderInstancing.cpp:70`)**

이 인스턴싱 그룹이 쓰는 모든 시퀀스를 순회해 최대 프레임 수를 구한 뒤(관절 수는 마지막 시퀀스 값으로 덮어씁니다 — 아래 한계 참조), `(시퀀스 × 관절 × 프레임)` 크기의 `TRANSFORM`(위치·쿼터니언·스케일) 배열을 한 번에 만들어 `StructuredBuffer`(t33)로 올립니다. 인덱싱은

```cpp
iIndex = (k * iMaxJoint + i) * iMaxFrame + j;   // k=시퀀스, i=관절, j=프레임
```

이후 매 프레임 GPU에 올라가는 건 인스턴스당 24바이트짜리 재생 상태(`BONEINSTDATA`: 시퀀스 ID, 현재/다음 프레임, 보간 비율, 루트모션 여부)뿐입니다.

**(2) 인스턴스별 본 행렬 계산 — `SequenceInst` 컴퓨트 (`ComputeShader.hlsl:109`)**

```hlsl
[numthreads(32, 32, 1)]
void SequenceInst(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= g_pBone[0].g_iBoneMaxJoint) return;

    float fRate = g_vecBoneBuffer[DTid.y].time;          // DTid.y = 인스턴스
    uint iRootIndex = g_vecBoneBuffer[DTid.y].frame
        + g_pBone[0].g_iBoneMaxFrame * g_pBone[0].g_iBoneMaxJoint
        * g_vecBoneBuffer[DTid.y].animationID;           // 시퀀스별 오프셋
    uint iIndex     = DTid.x * g_pBone[0].g_iBoneMaxFrame + iRootIndex;
    ...
    float4 quaternion = Slerp(g_vecBonePalette[iIndex].queternion,
                              g_vecBonePalette[iNextIndex].queternion, fRate);
    ...
    g_vecFinalBuffer[DTid.x + g_pBone[0].g_iBoneMaxJoint * DTid.y]
        = mul(g_vecBones[DTid.x], mul(matScale, mul(matRot, matPos)));
}
```

디스패치는 `(관절수/32, 인스턴스수/32)`입니다 (`RenderInstancing::Update()`, `:256`). 루트모션 시퀀스면 루트 관절의 위치를 빼서 제자리 재생으로 만듭니다.

**(3) 인스턴스 정점 버퍼 — `Drawable::GetInstData()` (`Drawable.cpp:211`), 입력 레이아웃 (`BindableManager.cpp:445`)**

인스턴스당 **312바이트** 고정 레이아웃입니다.

| 오프셋 | 크기 | 시맨틱 | 내용 |
|---|---|---|---|
| 0 | 192 | `World`/`View`/`LIGHTVP` × 4 | WVP, WorldView, 광원 VP 행렬 |
| 192 | 44 | `Material` 0~3 | 디퓨즈·스페큘러 색, 러프니스(2), 메탈릭 비율 |
| 236 | 64 | `JointSocket` 0~3 | 조인트 소켓 로컬 행렬 |
| 300 | 12 | `Bone` 0~2 | 인스턴스 ID, 소켓 관절 인덱스, 부모 관절 수 |

`PreRender()`(`:278`)에서 `D3D11_MAP_WRITE_DISCARD`로 매핑해 리스트를 순회하며 채웁니다.

**(4) 정점 셰이더 — `VS_SkinInst` (`anisotropic_microfacet.hlsl:156`)**

```hlsl
[unroll]
for (int i = 0; i < 4; ++i)
{
    pos += mul(float4(input.pos, 1.f),
        g_vecBones[input.blendIndex[i] + g_pBone[0].g_iBoneMaxJoint * iInstId]) * fWeight[i];
    ...
}
```

가중치는 3개만 정점에 싣고 네 번째는 `1 - w0 - w1 - w2`로 복원해 4바이트를 아낍니다.

**(5) 조인트 소켓(무기 부착)의 인스턴싱**

캐릭터가 인스턴싱되면 손에 든 무기도 같이 인스턴싱되어야 하는데, 무기의 위치는 부모 캐릭터의 본 행렬에 종속됩니다. `RenderInstancing::Update()`가 소켓에 붙은 자식 Drawable에게 부모의 인스턴스 인덱스와 관절 수를 심어주고(`SetInstID` / `SetParentJointCount`), **부모의 파이널 버퍼를 자식의 조인트 소켓 버퍼(t32)로 그대로 넘깁니다**(`:236`). 자식의 정점 셰이더는 같은 인덱싱 규칙으로 부모 관절 행렬을 찾아갑니다.

```hlsl
matWVP = mul(input.joint,
    mul(g_vecJointSockets[input.parentJoint + input.parentJointCount * input.instID], matWVP));
```

**(6) 인스턴싱 전환 기준**

`RenderManager::Update()`(`:475`)가 매 프레임 그룹 인원을 세서 **5마리 미만이면 인스턴싱 버킷을 해체하고 개별 드로우 리스트로 되돌립니다**. 인스턴스 버퍼 매핑·컴퓨트 디스패치 오버헤드가 이득보다 큰 구간을 피하기 위한 것입니다. 그룹 키는 `Drawable::UpdateInstanceKey()`(`:423`)가 메쉬·머티리얼·VS·PS·애니메이션·텍스처 태그 해시를 곱해 만듭니다.

### 막혔던 지점과 해결

`RenderInstancing::Update()` 끝(`:265~275`)과 `Animation::UpdateMatrix()`(`:714`), `Sequence::Update()`(`:311`)에 GPU 버퍼를 CPU로 읽어 내리는 `DebugBuffer` 호출이 각각 주석 처리된 채 남아 있습니다. 애니메이션 팔레트 / 스켈레톤 인버스 바인드 / 파이널 행렬을 따로따로 덤프해 본 흔적입니다. 인덱싱 규칙이 세 겹(시퀀스 × 관절 × 프레임)이라 어느 단계에서 어긋났는지 눈으로 볼 방법이 없어서, 단계별로 버퍼를 내려받아 대조하는 식으로 잡았습니다.

`RenderInstancing::AddDrawable()`(`:176`)의 인스턴스 버퍼 재할당도 같은 맥락입니다. 초기 정원 500을 넘으면 정원을 두 배로 늘리고 버퍼를 다시 만듭니다.

### 한계

- **스켈레톤 인버스 바인드 행렬을 정원(500)만큼 복제해서 올립니다**(`SetSkeleton`, `:125`). 컴퓨트 셰이더는 `g_vecBones[DTid.x]`로 관절 인덱스만 참조하므로 인스턴스별 복제가 필요 없습니다. 관절 65개 기준 `65 × 500 × 64B ≈ 2MB`를 낭비하고 있고, 한 벌만 올리면 4KB면 됩니다.
- 파이널 버퍼는 정원 × 관절 수로 잡혀 있어 실제 인원이 적어도 메모리를 그대로 씁니다.
- **정원 확장이 반쪽입니다.** `AddDrawable()`(`:176`)은 정원을 두 배로 늘린 뒤 `CreateInstBuffer()`만 다시 호출합니다. 스켈레톤 버퍼·본 데이터 버퍼·파이널 버퍼는 옛 정원 크기 그대로라, 501번째 인스턴스부터는 본 행렬 쓰기가 버퍼 범위를 벗어납니다. 확장 경로를 실제로 밟아 본 적이 없어 드러나지 않은 문제입니다.
- `CreateBoneBuffer()`(`:82`)가 `iMaxJoint`를 시퀀스마다 조건 없이 덮어씁니다(`iMaxJoint = pInfo->vecJoint.size()`). 프레임 수는 최댓값을 취하는데 관절 수는 마지막 시퀀스 값이 남습니다. 한 스켈레톤을 공유하는 시퀀스들이라 현재는 값이 같아서 문제가 없지만, 관절 수가 다른 시퀀스가 섞이면 바로 깨집니다.
- 그룹 객체와 GPU 버퍼는 프레임을 넘어 유지되지만 멤버 리스트는 매 프레임 비워졌다가(`RenderManager::Clear()` `:1004`) 컬링 결과로 다시 채워집니다. 그래서 인스턴스 버퍼는 프레임마다 전량 다시 매핑·기록됩니다. 변경된 인스턴스만 부분 갱신하는 최적화 여지가 남아 있습니다.
- 인스턴싱 경로에는 additive 블렌딩이 없습니다. 단일 캐릭터 경로(`Sequence` 컴퓨트)는 두 시퀀스를 섞지만 `SequenceInst`는 시퀀스 하나만 처리합니다.
- 상용 엔진은 보통 여기서 한 단계 더 나아가 본 행렬을 텍스처로 굽거나(애니메이션 텍스처) GPU 인스턴싱 + 컬링을 `DrawIndexedInstancedIndirect`로 GPU에서 완결시킵니다. 이 구현은 그룹 구성과 컬링이 전부 CPU에 있습니다.

### 참조 위치

| 대상 | 위치 |
|---|---|
| 인스턴싱 렌더 경로 | `Engine/Include/Render/RenderInstancing.cpp` — `CreateBoneBuffer()` `:70`, `SetSkeleton()` `:125`, `Update()` `:190`, `PreRender()` `:278`, `Draw()` `:302` |
| 그룹 구성·해체 | `Engine/Include/Render/RenderManager.cpp` — `AddDrawable()` `:68`, `Update()` `:475`, `PreRender()` `:535`, `RenderOpaqueInst()` `:640` |
| 인스턴스 데이터 패킹 | `Engine/Include/Bindable/Drawable.cpp` — `GetInstData()` `:211`, `UpdateInstanceKey()` `:423` |
| 입력 레이아웃(312B) | `Engine/Include/Bindable/BindableManager.cpp:445`, 등록 `:480` |
| 본 행렬 컴퓨트 | `Client/Bin/Resource/Shader/ComputeShader.hlsl` — `SequenceInst` `:109`, `Slerp` `:3` |
| 스키닝 정점 셰이더 | `Client/Bin/Resource/Shader/anisotropic_microfacet.hlsl` — `VS_SkinInst` `:156` |

---

## 2. 스크린 스페이스 데칼

**영상** https://www.youtube.com/watch?v=GkuhI-Q2rCY

### 개요

총알 자국이나 핏자국처럼 지형·캐릭터 어디에나 달라붙는 데칼을, 메쉬에 UV를 새로 만들지 않고 화면 공간에서 처리합니다. 데칼 하나가 육면체 하나이고, 그 육면체를 그리는 픽셀마다 "이 픽셀에 실제로 그려진 표면이 육면체 안에 들어 있는가"를 깊이 버퍼로 판정합니다. 데칼이 별도의 G-buffer에 재질을 기록하고 조명 패스가 그것을 원본 G-buffer와 합성하므로, 데칼도 노멀맵과 스페큘러를 갖고 조명을 정상적으로 받습니다.

### 기법 설명

1. 데칼 볼륨(단위 정육면체)을 일반 메쉬처럼 래스터라이즈한다. 깊이 테스트/쓰기는 끈다.
2. 각 픽셀에서 그 픽셀의 화면 좌표로 **씬 깊이**를 샘플링한다.
3. 깊이 + 화면 좌표 + 역투영으로 **뷰 공간 위치**를 복원한다. 이게 그 픽셀에 실제로 보이는 표면의 위치다.
4. 그 위치에 **역 world-view 행렬**을 곱해 데칼 로컬 공간으로 옮긴다.
5. `localpos.xz + 0.5`가 `[0, 1]²` 밖이면 `clip()`으로 버린다. 안이면 그 값이 곧 데칼 텍스처의 UV다. (이 구현은 XZ 두 축만 판정합니다 — 아래 한계 참조.)

### 구현

**좌표 변환 — `GetDecalUV()` (`Decal.fx:36`)**

```hlsl
float2 GetDecalUV(float2 uv, matrix matInvWorldView)
{
    float2 depth_uv = uv;
    depth_uv.y *= -1;
    depth_uv = depth_uv * 0.5f + 0.5f;                       // NDC → 텍스처 UV
    float depth = g_DepthTexture0.Sample(g_sPoint, depth_uv).x;

    // 투영 행렬 원소로 직접 역투영 (뷰 공간 z 복원 후 x, y 스케일 해제)
    float3 viewpos = float3(uv, g_matProj[3][2] / (depth - g_matProj[2][2]));
    viewpos.x /= g_matProj[0][0];
    viewpos.y /= g_matProj[1][1];
    viewpos.xy *= viewpos.z;

    float3 localpos = mul(float4(viewpos, 1.f), matInvWorldView);

    float2 decal_uv = localpos.xz + 0.5f;
    clip(decal_uv);          // 음수면 폐기
    clip(1.0 - decal_uv);    // 1 초과면 폐기
    return decal_uv;
}
```

`clip()`을 두 번 부호를 바꿔 호출해 `[0,1]` 범위를 판정합니다. `clip`은 인자에 음수 성분이 하나라도 있으면 픽셀을 버립니다.

**역 world-view 행렬 만들기 — `Decal::PostUpdate()` (`Decal.cpp:92`)**

행렬 역산을 일반 역행렬 계산으로 하지 않고, 각 성분의 역변환을 직접 조립합니다.

```cpp
m_tCBuffer.matInvWorldView =
      Graphics::GetInst()->GetCamera()->GetInvView()
    * Matrix::TranslateFromVector(-pTransform->GetPosition())
    * pTransform->GetRotationMatrix().Transpose()      // 회전의 역 = 전치
    * Matrix::Scaling(1.f / pTransform->GetScale());
```

**G-buffer 기록 — `PS_DECAL()` (`Decal.fx:62`)**

데칼 전용 MRT 4장에 원본 G-buffer와 같은 채널 배치로 씁니다. 알파에는 페이드 비율을 곱해 넣어서, 조명 패스가 이 알파로 원본과 보간합니다.

```hlsl
output.value0 = g_vDiffuseColor * decal_diffuse + g_vEmissiveColor * decal_emissive;
output.value1.xyz = BumpMapping(float3(0,1,0), float4(1,0,0,1), decal_normal.xyz) * 0.5f + 0.5f;
output.value1.w = decal_normal.w * fFadeRate;
...
output.value0.w *= fFadeRate;
```

노멀은 위 방향 `(0,1,0)` · 탄젠트 `(1,0,0)`을 고정 기준으로 삼아 접선 공간 노멀맵을 변환합니다. 데칼 볼륨의 로컬 XZ 평면이 곧 UV 평면이기 때문입니다.

**블렌드 스테이트 — `BindableManager.cpp:129`**

RT 4장에 독립 블렌드를 걸되, **색은 소스 알파 보간, 알파는 `MAX`** 로 설정했습니다.

```cpp
{true, SRC_ALPHA, INV_SRC_ALPHA, OP_ADD,      // 색
       SRC_ALPHA, DEST_ALPHA,    OP_MAX, ...} // 알파
```

데칼이 겹칠 때 색은 정상적으로 위에 덧칠되지만 커버리지(알파)는 누적되지 않고 가장 진한 값이 남습니다. 알파가 `ADD`면 데칼 두 장이 겹친 곳에서 알파가 1을 넘어 조명 패스 보간이 깨집니다.

**패스 구성 — `RenderManager::RenderDecal()` (`:929`)**

```cpp
pMRT->SetDepthSRV(10);        // 씬 깊이를 SRV로 (읽기 전용 바인딩)
m_pDecalMRT->SetTargets();    // 데칼 G-buffer를 RT로
m_pDecalBlend->Bind();
m_pNoDepthRead->Bind();       // 깊이 테스트 off
```

깊이 버퍼를 DSV가 아닌 SRV로 바인딩하는 게 핵심입니다. 같은 리소스를 RT와 SRV로 동시에 걸 수 없으므로 데칼 패스에서는 깊이 테스트를 포기하고 깊이를 텍스처로만 읽습니다.

**인스턴싱 변형 — `VS_DECAL_INST` / `PS_DECAL_INST` (`Decal.fx:15`, `:89`)**

데칼은 개수가 많아지기 쉬워서 인스턴싱 경로를 따로 만들었습니다. 인스턴스별 역 world-view 행렬(64바이트)과 재질·페이드 값을 정점 스트림으로 넘깁니다(`Decal::GetInstData()`, `Decal.cpp:45`, 스트라이드 200바이트). 전용 입력 레이아웃 `Decal_Inst`가 `InvWorldView` 시맨틱으로 행렬을 4행에 나눠 받습니다(`BindableManager.cpp:486`, 등록 `:514`).

**PBR 변형 — `PS_DECAL_PBR` (`Decal.fx:116`)**

스페큘러 슬롯을 오파시티로, 이미시브 슬롯을 러프니스로 재해석하는 변형입니다. 오파시티가 0이면 즉시 `clip(-1)`로 폐기해 불필요한 MRT 쓰기를 막습니다.

### 한계

- **깊이 축(로컬 Y)을 자르지 않습니다.** `GetDecalUV()`는 `localpos.xz`만 `clip()`하고 `localpos.y`는 검사하지 않습니다. 데칼 볼륨이 육면체가 아니라 로컬 Y축으로 무한히 뻗은 사각 기둥으로 동작하므로, 바닥에 찍은 데칼이 그 위에 서 있는 캐릭터나 아무리 멀리 있는 천장에도 그대로 칠해집니다. `clip(0.5 - abs(localpos.y))` 한 줄이면 되는데 빠져 있습니다.
- **표면 각도 판정이 없습니다.** 데칼 볼륨 안이기만 하면 벽이든 바닥이든 다 칠합니다. 상용 구현은 복원한 위치의 화면 미분(`ddx`/`ddy`)으로 월드 노멀을 얻어 데칼 방향과의 내적이 임계값 미만이면 버립니다. 그 결과 급경사면에서 텍스처가 늘어나는 스트레칭이 그대로 남습니다.
- 데칼이 캐릭터·동적 오브젝트에도 무차별로 칠해집니다. 스텐실 마스크로 정적 지오메트리만 골라내는 처리가 없습니다.
- 데칼 G-buffer가 RGBA8 4장 풀해상도 고정입니다. 데칼이 화면의 일부만 덮어도 매 프레임 전체를 클리어합니다.
- **카메라가 데칼 볼륨 안에 들어가면 데칼이 사라집니다.** 데칼 패스는 깊이 테스트만 끄고(`NoDepth` 깊이·스텐실 스테이트) 래스터라이저는 전역 기본값(`Basic`, `D3D11_CULL_BACK`)을 그대로 씁니다. 카메라가 볼륨 내부로 들어가면 앞면이 근평면에 잘려 그릴 것이 남지 않습니다. 앞면 컬링 + `GREATER` 깊이 테스트로 바꾸는 것이 표준 해법입니다.
- `Sample`로 깊이를 읽는데 MSAA 대응이 없습니다.

### 참조 위치

| 대상 | 위치 |
|---|---|
| 데칼 셰이더 | `Client/Bin/Resource/Shader/Decal.fx` — `GetDecalUV()` `:36`, `PS_DECAL()` `:62`, `VS_DECAL_INST()` `:15`, `PS_DECAL_INST()` `:89`, `PS_DECAL_PBR()` `:116` |
| CPU 측 | `Engine/Include/Bindable/Decal.cpp` — `GetInstData()` `:45`, `PostUpdate()` `:92`, `Bind()` `:113` |
| 렌더 패스 | `Engine/Include/Render/RenderManager.cpp` — `RenderDecal()` `:929` |
| 블렌드 스테이트 | `Engine/Include/Bindable/BindableManager.cpp:129` (`DecalBlend`) |
| 조명 패스의 합성 | `Client/Bin/Resource/Shader/anisotropic_microfacet.hlsl` — `PS_Multi()` `:461` |

---

## 3. GPU 파티클

**영상** https://www.youtube.com/watch?v=FVUxQUKp16M · https://www.youtube.com/watch?v=aGEj5LkXI30

### 개요

파티클의 생성·소멸·물리 적분을 전부 GPU에서 처리합니다. CPU는 "이번 프레임에 몇 개를 새로 만들라"는 숫자만 넘기고, 파티클 상태 배열은 GPU 메모리를 벗어나지 않습니다. 그리기도 정점 버퍼 없이 인스턴스 드로우 한 번으로 끝내고, 지오메트리 셰이더가 점 하나를 카메라를 향한 사각형으로 전개합니다. 컴퓨트 → 정점 → 지오메트리 → 픽셀 네 단계를 관통하는 구조입니다.

### 기법 설명

파티클 배열을 `RWStructuredBuffer<Particle>`에 두고, 컴퓨트 셰이더 스레드 하나가 파티클 하나를 맡습니다. 각 스레드는 자기 파티클이 죽어 있으면 "생성 예산"을 하나 소비해 되살리고, 살아 있으면 나이를 더해 적분합니다. 생성 예산은 스레드 그룹마다 `RWStructuredBuffer<int>` 한 칸으로 두고, 여러 스레드가 동시에 예산을 가져가려 하므로 원자적 연산으로 경합을 처리합니다.

정점 버퍼는 아예 만들지 않습니다. `DrawInstanced(1, maxParticleCount)`로 정점 1개짜리 인스턴스를 파티클 수만큼 그리고, 정점 셰이더는 `SV_InstanceID`만 다음 단계로 넘깁니다. 지오메트리 셰이더가 그 ID로 파티클 상태를 읽어 사각형 두 삼각형을 만듭니다.

### 구현

**(1) 생성 예산 분배 — `Particle::Update()` (`Particle.cpp:48`)**

방출 주기가 지날 때마다 생성 개수를 세고, 그것을 스레드 그룹 수로 나눠 분배합니다. 나머지는 그룹에 한 개씩 얹는데, **매번 같은 그룹에 얹지 않도록 시작 오프셋을 프레임마다 회전시킵니다.**

```cpp
for (int i = 0; i < iGroupCount; ++i)
    vecCreateCount[i] = iCreateCount / iGroupCount;

for (int i = m_iPrevCreateGroupOffset;
     i < m_iPrevCreateGroupOffset + iCreateCount % iGroupCount; ++i)
    ++vecCreateCount[i % iGroupCount];

m_iPrevCreateGroupOffset =
    (m_iPrevCreateGroupOffset + iCreateCount % iGroupCount) % iGroupCount;
```

오프셋을 고정하면 앞쪽 그룹의 파티클만 계속 재생성되어 방출이 배열 앞부분에 몰립니다.

**(2) 생성·적분 컴퓨트 — `CS_PARTICLE` (`Particle.fx:33`, `numthreads(64,1,1)`)**

```hlsl
if (!g_vecParticleInfo[iDispatchThreadID.x].alive)
{
    if (g_vecEmitter[iGroupID.x] > 0)
    {
        int iOriginValue = 0;
        InterlockedCompareExchange(g_vecEmitter[iGroupID.x],
            g_vecEmitter[iGroupID.x], g_vecEmitter[iGroupID.x] - 1, iOriginValue);

        if (iOriginValue == g_vecEmitter[iGroupID.x] + 1)
        {
            // 이 스레드가 예산을 획득 → 파티클 초기화
        }
    }
}
else
{
    g_vecParticleInfo[i].age += g_fGlobalDeltaTime;
    ...
    pos   += speed * dt;
    speed += g_vParticleAccelation * dt;
    color  = lerp(startColor, endColor, ratio);
    size   = lerp(startSize,  endSize,  ratio);
    frame  = (int)(age / maxage * g_iParticleMaxFrame);
}
```

`InterlockedCompareExchange`의 반환값(교환 전 원본)과 갱신 후 값을 비교해 "내가 실제로 감소시킨 스레드인지"를 판정합니다. 스폰 위치·수명·속도는 최소/최대 파라미터 사이를 난수로 보간하고, 속도는 정규화해서 방향만 취한 뒤 가속도로 가속시킵니다.

**(3) 정점 없는 드로우 — `Particle::Bind()` (`Particle.cpp:167`)**

```cpp
m_pBuffer->SetSRV(40);                                   // 파티클 상태를 SRV로 전환
ctx->IASetInputLayout(nullptr);
ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
ctx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
ctx->DrawInstanced(1, m_tCBuffer.iMaxParticleCount, 0, 0);
```

**(4) 빌보드 전개 — `GS_PARTICLE` (`Particle.fx:103`, `maxvertexcount(6)`)**

죽은 파티클은 정점을 하나도 내보내지 않고 `return`합니다(GPU 측 컬링). 살아 있으면 **뷰 공간에서 좌우/상하로 사이즈만큼 벌린 뒤 투영**합니다. 뷰 공간에서 벌리므로 사각형이 항상 카메라를 정면으로 봅니다.

```hlsl
_point[i + j*2].pos = mul(float4(g_vecParticle[id].pos, 1.f), g_matView);
_point[i + j*2].pos.x += (i*2 - 1) * g_vecParticle[id].size.x / 2.f;
_point[i + j*2].pos.y -= (j*2 - 1) * g_vecParticle[id].size.y / 2.f;
_point[i + j*2].pos = mul(_point[i + j*2].pos, g_matProj);
```

UV는 스프라이트 시트 프레임 인덱스로 계산해 플립북 애니메이션을 만듭니다.

### 막혔던 지점과 해결

`Particle::Update()`의 `#ifdef _DEBUG` 블록(`Particle.cpp:110~164`)에 **파티클 생사 검증 코드가 통째로 주석 처리된 채** 남아 있습니다. 상태 버퍼와 이미터 버퍼를 매 프레임 CPU로 리드백해 이번 프레임의 birth / death / live / dead 수를 세고, 요청한 생성 개수와 실제로 소비된 예산의 차이를 `OutputDebugString`으로 찍는 코드입니다.

```cpp
_stprintf_s(strDebug,
  TEXT("Create Count: %d, birth count: %d, death count: %d, live: %d, dead: %d, total: %d\n"),
  iCreateCount - iPostCreateCount, iBirthCount, iDeathCount, iLiveCount, iDeadCount,
  iLiveCount + iDeadCount);
```

원자적 예산 소비가 맞게 도는지(중복 소비로 파티클이 과다 생성되거나, 경합에 밀려 예산이 남는지)를 눈으로 볼 방법이 없어서 만든 계측입니다. `iLiveCount + iDeadCount`가 총 개수와 맞는지까지 확인하는 걸 보면 배열 일관성 자체를 의심했던 것으로 보입니다.

### 한계

- **난수 생성이 편향돼 있습니다.** `Random()`(`Particle.fx:12`)이 `cos(seed)`, `sin(seed)`로 만든 UV로 노이즈 텍스처를 샘플링한 뒤 5×5 가우시안으로 블러합니다. 블러가 분산을 줄이므로 결과가 노이즈 평균값 쪽으로 쏠려 파티클이 스폰 박스 중앙에 뭉치고, `cos`/`sin` UV는 arcsine 분포라 텍스처 가장자리를 과샘플링합니다. 시드도 `seed *= threadID`라 스레드 0은 항상 같은 값을 받습니다. 정수 해시 기반 균일 난수로 교체해야 합니다.
- 속도 파라미터가 `normalize()`되어 크기가 버려지므로, 설정한 속도 값은 방향 분포로만 동작합니다. 파티클은 항상 1 unit/s로 스폰된 뒤 가속도로만 빨라집니다.
- 소프트 파티클 처리가 없어서 파티클과 씬 지오메트리가 만나는 곳에 경계선이 그대로 보입니다.
- 정렬이 없어서 알파 블렌딩 순서가 배열 인덱스 순입니다. 반투명 파티클이 겹치면 순서가 틀립니다.
- 파티클 시스템마다 상태 버퍼와 이미터 버퍼를 따로 갖습니다. 이펙트가 많아지면 디스패치 호출이 그만큼 늘어납니다. 상용 구현은 시스템을 하나의 버퍼에 모으고 `DispatchIndirect`로 살아있는 개수만큼만 돕니다.

### 참조 위치

| 대상 | 위치 |
|---|---|
| 파티클 셰이더 | `Client/Bin/Resource/Shader/Particle.fx` — `Random()` `:12`, `CS_PARTICLE` `:33`, `VS_PARTICLE` `:93`, `GS_PARTICLE` `:103`, `PS_PARTICLE` `:141` |
| CPU 측 | `Engine/Include/Bindable/Particle.cpp` — 생성자 `:23`, `Update()` `:48`, `Bind()` `:167` |
| 버퍼 래퍼 | `Engine/Include/Shader/StructuredBuffer.cpp`, `Engine/Include/Bindable/ComputeShader.cpp` |
| 레지스터 정의 | `Client/Bin/Resource/Shader/shared.hlsl` — `g_vecParticle` t40, `g_vecParticleInfo` u2, `g_vecEmitter` u3, `cbuffer Particle` b7 |

---

## 4. 애니메이션 블렌딩 (GPU 본 행렬 계산)

**영상** https://www.youtube.com/watch?v=6BggfhaLDms

### 개요

본 행렬 팔레트 계산을 CPU가 아니라 컴퓨트 셰이더에서 합니다. 관절 하나에 스레드 하나를 배정해 키프레임 사이를 보간하고, 회전은 쿼터니언 slerp로 섞습니다. 여기에 additive 시퀀스를 하나 더 얹어 **관절마다 다른 가중치**로 섞을 수 있게 했습니다. 하체는 이동 애니메이션을 유지하고 상체만 공격 모션으로 덮는 식의 부위별 블렌딩이 목적입니다.

### 기법 설명

한 관절의 한 프레임 포즈는 위치·쿼터니언·스케일 세 값입니다. 재생 시각이 프레임 사이에 있으면 위치·스케일은 선형 보간, 회전은 구면 선형 보간(slerp)합니다. 회전을 행렬이나 오일러각으로 보간하면 중간 자세가 찌그러지거나 짐벌락이 생기므로 쿼터니언을 씁니다.

두 시퀀스를 섞을 때는 각각 보간한 결과를 다시 한 번 블렌드 계수로 섞습니다. 이때 계수를 관절마다 다르게 주면 부위별 블렌딩이 됩니다.

최종 스키닝 행렬은 `invBindPose(i) · pose(i)` 입니다. `invBindPose`는 바인드 포즈에서 정점을 관절 로컬 공간으로 되돌리는 행렬로, 스켈레톤에 미리 구워둡니다.

### 구현

**(1) 리소스 배치 — `Animation::Bind()` (`Animation.cpp:422`)**

```cpp
m_pMidBuffer->SetUAV(0);      // g_vecFinalBuffer  : invBindPose * pose
m_pPoseBuffer->SetUAV(1);     // g_vecPoseBuffer   : pose만 (IK·소켓용)
UpdateMatrix();               // Sequence 컴퓨트 디스패치
m_pMidBuffer->ResetUAV(0);
m_pPoseBuffer->ResetUAV(1);
MatrixPostProcess();          // IK
SetFinalBuffer();             // m_pFinalBuffer → t30 (g_vecBones)
```

관절 행렬 버퍼가 세 개(`Mid` / `Pose` / `Final`)인 이유는, 스키닝용 최종 행렬과 IK·조인트 소켓이 필요로 하는 순수 포즈 행렬이 다르기 때문입니다. 소켓은 관절의 월드 위치가 필요하지 `invBindPose`가 곱해진 값이 필요하지 않습니다.

**(2) 블렌드 팔레트 전송 — `Animation::UpdateMatrix()` (`Animation.cpp:677`)**

```cpp
m_tBoneCBuffer.iSequenceCount = 1;
m_pCurrentSequence->pSequence->Update(m_pCurrentSequence->fTime, 31);   // t31
m_tBoneCBuffer.pInfo[0] = m_pCurrentSequence->pSequence->GetBoneInfo();

if (m_pAdditiveSequence)
{
    m_pAdditiveSequence->pSequence->Update(m_pAdditiveSequence->fTime, 36, 1);  // t36
    const std::vector<float>& vecPalette = m_pAdditiveSequence->pSequence->GetBlendPalette();
    m_tBoneCBuffer.pInfo[1] = m_pAdditiveSequence->pSequence->GetBoneInfo();
    memcpy_s(m_tBoneCBuffer.pBlendPallete, 256 * 4, &vecPalette[0], 4 * vecPalette.size());
    ++m_tBoneCBuffer.iSequenceCount;
}
```

관절별 블렌드 계수 256개를 상수 버퍼 b4에 `float4[64]`로 실어 보냅니다. HLSL 상수 버퍼는 배열 원소가 16바이트 정렬되므로 `float[256]`으로 선언하면 4KB를 쓰게 됩니다. `float4[64]`로 묶어 1KB로 줄이고, 셰이더에서 `[DTid.x / 4][DTid.x % 4]`로 풀어 씁니다.

**(3) 블렌딩 컴퓨트 — `Sequence` (`ComputeShader.hlsl:31`, `numthreads(32,1,1)`)**

```hlsl
float3 pos   = (g_vecTransforms[iIndex].pos - g_pBone[0].g_vBoneRootPos) * (1 - fRate)
             + (g_vecTransforms[iNextIndex].pos - g_pBone[0].g_vBoneRootPos) * fRate;
float4 quaternion = Slerp(g_vecTransforms[iIndex].queternion,
                          g_vecTransforms[iNextIndex].queternion, fRate);

if (g_iBoneSequenceCount > 1)
{
    // additive 시퀀스도 같은 방식으로 보간
    float fBlend;
    if (g_pBone[1].g_fBoneMaxTime - g_pBone[1].g_fBoneBlendMaxTime < g_pBone[1].fSequenceTime)
    {                                                     // 블렌드 아웃
        fBlend = (g_pBone[1].g_fBoneMaxTime - g_pBone[1].fSequenceTime)
               / g_pBone[1].g_fBoneBlendMaxTime;
        fBlend = g_pBoneAdditiveBlend[DTid.x / 4][DTid.x % 4] * clamp(fBlend, 0.f, 1.f);
    }
    else                                                  // 블렌드 인
    {
        fBlend = g_pBoneAdditiveBlend[DTid.x / 4][DTid.x % 4]
               * clamp(g_pBone[1].fSequenceTime / g_pBone[1].g_fBoneBlendMaxTime, 0.f, 1.f);
    }

    pos        = pos   * (1.f - fBlend) + pos2   * fBlend;
    scale      = scale * (1.f - fBlend) + scale2 * fBlend;
    quaternion = Slerp(quaternion, quaternion2, fBlend);
}

g_vecPoseBuffer[DTid.x]  = mul(matScale, mul(matRot, matPos));
g_vecFinalBuffer[DTid.x] = mul(g_vecBones[DTid.x], g_vecPoseBuffer[DTid.x]);
```

시간 기반 블렌드 인/아웃 계수에 관절별 팔레트를 곱하는 구조입니다. 즉 "이 관절이 additive를 얼마나 받는가"(팔레트) × "지금 얼마나 섞일 때인가"(시간)입니다.

`Slerp()`(`:3`)는 내적이 음수면 한쪽 쿼터니언을 반전시켜 짧은 호를 타게 하고, `sin(theta)`가 0에 가까우면 보간을 포기하고 원본을 반환합니다.

**(4) 루트 모션 — `Sequence::Update()` (`Sequence.cpp:274`)**

루트모션 시퀀스면 루트 관절(0번)의 보간된 위치를 계산해 상수 버퍼에 싣고, 컴퓨트에서 모든 관절 위치에서 그 값을 빼 제자리 재생으로 만듭니다. 실제 이동은 게임 코드가 담당합니다.

**(5) 시퀀스 전환 — `Animation::Update()` (`Animation.cpp:313`)**

시퀀스가 끝나면 루프 여부를 보고 시간을 되감거나(`fNextTime -= (int)(fNextTime / maxTime) * maxTime`), 다음 시퀀스가 지정돼 있으면 전환하거나, 마지막 프레임 직전에 멈춥니다. 노티파이는 지정 시각에 도달하면 콜백을 한 번 쏘고, 루프가 돌 때 `Notify::Clear()`로 재무장됩니다(`Engine/Include/Animation/Notify.cpp`).

**(6) IK — `Animation::MatrixPostProcess()` (`Animation.cpp:726`)**

FABRIK(Forward And Backward Reaching IK)을 CPU에서 5회 반복합니다. 포즈 버퍼를 리드백해 관절 위치만 뽑고, 목표 지점에서 루트 방향으로 한 번(backward), 루트에서 말단 방향으로 한 번(forward) 훑으면서 관절 간 거리를 유지한 채 위치를 당깁니다. 결과를 포즈 버퍼에 다시 쓰고, CPU에서 `invBindPose`를 곱해 `m_pMidBuffer`에 올린 뒤 `PostProcess` 컴퓨트로 파이널 버퍼에 복사합니다.

### 막혔던 지점과 해결

`Animation::UpdateMatrix()`의 디스패치 직후(`:714~718`)와 `Sequence::Update()`(`:311`)에 GPU 버퍼를 CPU로 읽어 내리는 리드백 호출이 주석 처리된 채 남아 있습니다. 각각 파이널 행렬 버퍼와 시퀀스 트랜스폼 버퍼를 덤프해 본 흔적입니다. 버퍼 세 개(Mid/Pose/Final)가 UAV와 SRV를 오가고 t30 슬롯을 `m_pMidBuffer`와 `m_pFinalBuffer`가 번갈아 쓰는 구조라, 어느 단계에서 값이 깨졌는지는 버퍼를 직접 내려받아 보는 것 말고는 좁힐 방법이 없었습니다.

`ToneMapping`류의 NaN 방어와 마찬가지로, `Slerp()`의 `sin_theta` 0 근처 가드도 같은 성격의 방어입니다. 쿼터니언 두 개가 거의 같을 때 `0/0`이 나와 관절 하나가 NaN이 되면 그 관절에 물린 정점 전체가 화면 밖으로 날아갑니다.

### 한계

- **IK가 위치만 갱신합니다.** `vecKeyFrame[i][0..2].w`(이동 성분)만 덮어쓰고 회전은 손대지 않습니다. 관절이 회전하는 게 아니라 늘어나므로 메쉬가 찌그러집니다. 실제로 IK 등록 경로가 데모에 거의 쓰이지 않습니다.
- **IK가 없어도 매 프레임 GPU 리드백이 발생합니다.** `MatrixPostProcess()`는 조건 없이 `m_pPoseBuffer->ReadBuffer()`를 호출하는데, 이건 `CopyResource` + `Map(READ)`라 GPU 동기 스톨입니다. IK 목록이 비었으면 이 블록 전체를 건너뛰어야 합니다.
- 블렌딩이 **베이스 1개 + additive 1개**로 고정입니다(`g_pBone[2]`). 블렌드 트리나 상태 기계가 아니라 슬롯 두 개짜리 구조라, 걷기↔달리기 속도 블렌딩 같은 걸 하려면 시퀀스를 미리 만들어야 합니다.
- 관절별 블렌드 팔레트는 API만 있고 실제 데모는 전 관절에 1.0을 넣습니다(`Editor/Include/Object/Player.cpp:52`). 부위별 마스킹을 실제로 튜닝해 본 적은 없습니다.
- 시퀀스 전환에 크로스페이드가 없습니다. `ChangeSequence()`는 즉시 교체이고, 블렌드 인/아웃은 additive 슬롯에만 있습니다.
- `additive`라는 이름을 쓰지만 실제 연산은 가산이 아니라 보간(lerp/slerp)입니다. 진짜 additive 블렌딩은 레퍼런스 포즈와의 차분을 더하는 방식입니다.

### 참조 위치

| 대상 | 위치 |
|---|---|
| 애니메이션 컴포넌트 | `Engine/Include/Bindable/Animation.cpp` — `Update()` `:313`, `Bind()` `:422`, `UpdateMatrix()` `:677`, `MatrixPostProcess()` `:726`, `SetFinalBuffer()` `:999` |
| 시퀀스 | `Engine/Include/Animation/Sequence.cpp` — `Update()` `:274`, `GetBlendPalette()` `:253`, `SetBlendFactor()` `:258` |
| 스켈레톤 | `Engine/Include/Animation/Skeleton.cpp`, 조인트 소켓 `Engine/Include/Animation/JointSocket.cpp` |
| 블렌딩 컴퓨트 | `Client/Bin/Resource/Shader/ComputeShader.hlsl` — `Slerp()` `:3`, `Sequence` `:31`, `PostProcess` `:161` |
| 스키닝 정점 셰이더 | `Client/Bin/Resource/Shader/anisotropic_microfacet.hlsl` — `VS_Skin` `:116` |
| 상수 버퍼 정의 | `Client/Bin/Resource/Shader/shared.hlsl` — `cbuffer Bone` b4 `:245` |

---

## 5. 검광 (Trail)

**영상** https://www.youtube.com/watch?v=kQUuy2z8A6U

### 개요

칼을 휘두를 때 칼날이 지나간 자리에 남는 잔상입니다. 칼끝과 칼밑 두 점을 받아 정점 스트립에 밀어 넣어 리본 메쉬를 만들고, 이 리본을 화면 전체가 아니라 전용 렌더 타깃에 따로 그린 뒤 컴퓨트 셰이더로 블러해서 HDR 타깃에 합성합니다. 샘플링 시점은 프레임이 아니라 **애니메이션 노티파이**가 잡습니다. 렌더 패스를 하나 추가해 파이프라인에 끼워 넣은 사례입니다.

### 기법 설명

리본은 정점 쌍(위/아래)의 배열입니다. 샘플이 하나 들어올 때마다 배열을 두 칸씩 뒤로 밀고 맨 앞에 현재 칼날 위치를 넣으면, 배열이 곧 최근 N개 샘플의 궤적이 됩니다. UV의 u를 배열 위치에 비례시키면 궤적을 따라 텍스처가 흐릅니다.

샘플을 언제 뽑느냐가 관건입니다. 렌더 프레임마다 뽑으면 프레임 레이트에 따라 궤적 길이가 달라지므로, 여기서는 **공격 애니메이션에 등간격 노티파이를 심어** 애니메이션 시간 기준으로 샘플링합니다.

번짐 효과는 이 리본만 별도 타깃에 그린 다음 그 타깃을 블러하고 원본 화면에 더하는 방식으로 냅니다. 씬 전체를 블러하면 안 되므로 타깃을 분리하는 것이 핵심입니다.

### 구현

**(1) 리본 메쉬 — `Trail::Trail(int iCount)` (`Trail.cpp:9`)**

정점 개수는 짝수여야 하고(`assert(iCount % 2 == 0)`), 짝수 인덱스가 윗줄 홀수 인덱스가 아랫줄입니다. UV는 생성 시 한 번만 채우고 이후 바꾸지 않습니다.

```cpp
m_vecVertex[i*2  ].uv = { (i*2) / (float)((n/2) - 1), 0 };
m_vecVertex[i*2+1].uv = { (i*2) / (float)((n/2) - 1), 1 };
```

인덱스 버퍼는 사각형마다 삼각형 두 개로 미리 구성합니다. 메쉬는 `D3D11_USAGE_DYNAMIC`으로 만듭니다.

**(2) 샘플링 시점 — 애니메이션 노티파이 (`Client/Include/Object/Player.cpp:673~750`)**

트레일은 정점 10개(사각형 5개)로 만들고, 공격 시퀀스 `"CharacterArmature|Sword_Slash"`에 **0.01666초 간격 노티파이 77개**를 심습니다.

```cpp
m_pTrail = GetScene()->CreateDrawable<Trail>("Trail", GetScene()->FindLayer(DEFAULT_LAYER), 10);

for (int i = 0; i < 77; ++i)
{
    std::string strNotify = "trail" + std::to_string(i);
    auto pNotify = GetAnimation()->AddNotify("CharacterArmature|Sword_Slash", strNotify, i * 0.01666f);
    ...
}
```

0번 노티파이만 `SetAllPosition()`으로 리본 전체를 현재 칼날 위치로 채워 초기화하고 트레일을 켭니다(휘두르기 시작 시 이전 궤적이 남지 않도록). 나머지 76개는 `SetPosition()`으로 샘플을 하나씩 밀어 넣습니다. 칼날 두 점은 칼 로컬 좌표 `(0, 2.0, 0)` / `(0, 0.3, 0)`을 무기 트랜스폼(조인트 소켓이 매 프레임 갱신)으로 변환해 얻습니다.

**(3) 궤적 갱신 — `Trail::SetPosition()` (`Trail.cpp:48`)**

```cpp
for (int i = (int)m_vecVertex.size() - 1; i >= 2; --i)
    m_vecVertex[i].pos = m_vecVertex[i - 2].pos;   // 두 칸씩 뒤로

m_vecVertex[0].pos = vTop;
m_vecVertex[1].pos = vBottom;

SetNormals(m_vecVertex, m_vecIndex);
SetTangent(m_vecVertex, m_vecIndex);

m_pMesh->SetVertexBuffer(0, &m_vecVertex[0], sizeof(VertexStandard) * m_vecVertex.size());
```

리본 모양이 샘플마다 바뀌므로 노멀과 탄젠트를 인덱스 버퍼 기준으로 다시 계산하고, 동적 정점 버퍼를 통째로 갱신합니다.

**(4) 렌더 레이어 지정 — `Trail::Init()` (`Trail.cpp:64`)**

```cpp
SetRenderLayer(Engine::RENDER_LAYER::BLUR);
FindAndAddBind<VertexShader>("anisotropic_microfacet VSNoSkin");
FindAndAddBind<PixelShader>("AlphaNoUVPS");
FindAndAddBind<RasterizerState>(CULL_NONE);
```

칼이 어느 방향으로 지나가든 리본의 앞뒷면이 뒤집히므로 컬링을 끕니다.

**(5) 블러 패스 — `RenderManager::RenderBlur()` (`RenderManager.cpp:1141`)**

```cpp
m_pBlurTarget->SetTargets(pMRT->GetDSV());   // 별도 RT + 씬 깊이 버퍼 재사용
m_pDestAlpha->Bind();                        // ONE / DEST_ALPHA

// BLUR 레이어 드로우

m_pBlurTarget->SetSRV(0, 0);
m_pBlurTexture->SetUAV(0);
m_pBlurCS->Dispatch(ceil(width * height / 1024.f));   // 5x5 가우시안
m_pBlurTexture->ResetUAV(0);

m_pAlphaBlend->Bind();
m_pBlurTexture->Bind();
pBlurNullVertexShader->Bind();  pBlurNullPixelShader->Bind();
m_pHDRTexture->SetTargets();
ctx->IASetPrimitiveTopology(TRIANGLESTRIP);
ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
ctx->Draw(4, 0);                                       // 풀스크린 합성
```

세 가지가 눈여겨볼 지점입니다.

- **씬 깊이 버퍼를 그대로 넘겨받습니다**(`SetTargets(pMRT->GetDSV())`). 색 타깃만 바꾸고 깊이는 공유하므로, 리본이 씬 지오메트리에 가려지는 것은 정상 처리됩니다.
- 블러는 컴퓨트로 `RWTexture2D`에 씁니다(`Blur`, `Particle.fx:150`, `numthreads(1024,1,1)`). 1차원 디스패치로 픽셀 인덱스를 받아 2D 좌표로 환산합니다.
- 합성은 정점 버퍼 없이 `NullVS`가 `SV_VertexID`로 화면 사각형을 만들어 `Draw(4, 0)` 한 번입니다.

### 한계

- **블러 타깃이 LDR(RGBA8)입니다.** 씬은 `R16G16B16A16_FLOAT`인데 검광만 8비트 타깃을 거쳐 합성되므로, 1.0을 넘는 밝기를 표현할 수 없어 블룸을 태우지 못합니다. 검광은 밝게 타오르는 게 자연스러운 이펙트라 이 부분이 가장 아쉽습니다.
- **블러가 분리 가능(separable) 구현이 아닙니다.** 5×5 커널을 한 패스에서 25탭으로 돕니다. 가로/세로 두 패스로 나누면 10탭으로 줄고, 그룹 공유 메모리를 쓰면 텍스처 페치도 크게 줍니다(같은 저장소의 블룸 필터는 실제로 그렇게 되어 있습니다).
- 블러 컴퓨트가 해상도를 `g_vDownScaleResolution * 4`로 역산합니다. HDR 다운스케일 상수 버퍼에 묶여 있어 독립적으로 해상도를 바꿀 수 없습니다.
- **프레임이 떨어지면 궤적이 중복 샘플로 채워집니다.** 노티파이는 애니메이션 시간 기준이라 한 프레임 안에 여러 개가 한꺼번에 발화할 수 있는데, 콜백이 전부 *같은* 무기 트랜스폼(그 프레임의 값)을 읽습니다. 결과적으로 리본에 같은 위치가 여러 번 들어가 궤적이 짧아집니다. 프레임 간 트랜스폼을 보간해서 샘플을 만들어야 맞습니다.
- 노티파이 77개를 이름 문자열(`"trail0"`~`"trail76"`)로 하나씩 등록하는 방식이라, 다른 공격 모션에 트레일을 붙이려면 그 시퀀스 길이에 맞춰 노티파이를 다시 심어야 합니다. 트레일이 스스로 시간을 누적해 샘플링하는 구조가 아닙니다.
- 리본 정점이 10개(사각형 5개)뿐이라 빠른 휘두르기에서 궤적이 각져 보입니다. 상용 구현은 샘플 사이를 스플라인으로 보간해 세분합니다.
- 궤적 나이에 따른 페이드가 없습니다. UV는 있지만 알파를 시간으로 깎지 않아 리본이 균일한 밝기로 남습니다.

### 참조 위치

| 대상 | 위치 |
|---|---|
| 리본 메쉬 | `Client/Include/Object/Trail.cpp` — 생성자 `:9`, `SetAllPosition()` `:39`, `SetPosition()` `:48`, `Init()` `:64` |
| 샘플링 노티파이 | `Client/Include/Object/Player.cpp:673~750` (트레일 생성 `:673`, 노티파이 77개 등록 `:675`, 콜백 `:686` / `:733`) |
| 블러 패스 | `Engine/Include/Render/RenderManager.cpp` — `RenderBlur()` `:1141`, 리소스 생성 `:445~453` |
| 블러 컴퓨트 · 합성 셰이더 | `Client/Bin/Resource/Shader/Particle.fx` — `Blur` `:150`, `NullVS` `:203`, `NullPS` `:213` |
| 블렌드 스테이트 | `Engine/Include/Bindable/BindableManager.cpp:132` (`DestAlpha`) |

---

## 6. 유체 시뮬레이션

**영상** https://www.youtube.com/watch?v=SVmCcZTgb9g

### 개요

수면을 2차원 높이장으로 두고 파동방정식을 컴퓨트 셰이더로 풉니다. 높이 버퍼 3개를 링으로 돌려 이전·현재 상태에서 다음 상태를 구하고, 정점 셰이더가 그 버퍼를 직접 읽어 평면 격자를 위아래로 변위시킵니다. 노멀은 이웃 높이의 중앙차분으로 그 자리에서 만듭니다.

### 기법 설명

파동방정식 `∂²z/∂t² = c²∇²z − μ·∂z/∂t` 를 시간·공간 모두 유한차분으로 이산화하면, 다음 시각의 높이가 현재·이전 시각의 높이와 네 이웃의 현재 높이로 표현됩니다.

```
z(i,j,k+1) = c1·z(i,j,k) + c2·z(i,j,k−1)
           + c3·( z(i+1,j,k) + z(i−1,j,k) + z(i,j+1,k) + z(i,j−1,k) )
```

이 형태는 이전 시각의 격자 전체를 들고 있어야 하므로 버퍼가 최소 3개 필요합니다(이전 / 현재 / 다음). 명시적 방법이라 시간 간격과 격자 간격, 파동 속도 사이에 안정 조건이 있고, 이를 넘기면 값이 발산합니다.

### 구현

**(1) 계수 계산과 안정 조건 — `Fluid::Fluid()` (`Fluid.cpp:20`)**

```cpp
assert(c < d / 2 / t * sqrtf(mu * t + 2.f));
assert(t < (mu + sqrtf(mu * mu + 32 * c * c / d / d)) / (8.f * c * c / d / d));

m_tCBuffer.c1 = (4.f - 8.f * c * c * t * t / d / d) / (mu * t + 2.f);
m_tCBuffer.c2 = (mu * t - 2.f) / (mu * t + 2.f);
m_tCBuffer.c3 = 2 * c * c * t * t / d / d / (mu * t + 2.f);
```

`d`=격자 간격, `mu`=감쇠, `c`=파동 속도, `t`=시간 간격입니다. 두 `assert`가 각각 파동 속도 상한과 시간 간격 상한(CFL 조건)입니다. 파라미터를 잘못 넣으면 화면이 깨지는 게 아니라 디버그 빌드에서 바로 걸립니다. 시간 간격 기본값은 `FIXED_UPDATE_TIME`(1/60초)이고, 시뮬레이션은 가변 프레임이 아니라 고정 스텝 루프에서 돕니다(`Window.cpp:442`의 누산기).

**(2) 버퍼 링 — `Fluid::FixedUpdate()` (`Fluid.cpp:121`)**

```cpp
int iNextBuffer = (m_iCurrentBuffer + 1) % FLUID_BUFFER_COUNT;   // 3

m_pBuffer[(m_iCurrentBuffer + 2) % 3]->SetSRV(38);   // 이전
m_pBuffer[ m_iCurrentBuffer         ]->SetSRV(39);   // 현재
m_pBuffer[ iNextBuffer              ]->SetUAV(4);    // 다음

m_pCS->Dispatch(width / 32 + (width % 32 != 0), height / 32 + (height % 32 != 0));

// ... 언바인드 ...
m_iCurrentBuffer = iNextBuffer;
```

같은 리소스를 SRV와 UAV로 동시에 걸 수 없으므로 매 스텝 인덱스만 회전시키고 바인딩을 교체합니다.

**(3) 시뮬레이션 커널 — `CS_FLUID` (`ComputeShader.hlsl:197`, `numthreads(32,32,1)`)**

```hlsl
if (g_iFluidWidth <= iDispatchThreadID.x) return;
else if (iDispatchThreadID.x == 0 || iDispatchThreadID.x == g_iFluidWidth - 1)
{
    g_vecHeightField[index] = 0.0;   // 좌우 경계 고정
    return;
}

g_vecHeightField[index] =
      g_fFluidc1 * g_vecCurrentHeightField[index]
    + g_fFluidc2 * g_vecPrevHeightField[index]
    + g_fFluidc3 * ( g_vecCurrentHeightField[indexleft]  + g_vecCurrentHeightField[indexright]
                   + g_vecCurrentHeightField[indexup]    + g_vecCurrentHeightField[indexdown] );
```

**(4) 정점 변위와 노멀 — `VS_FLUID` (`VertexShader.hlsl:66`)**

정점 셰이더가 `SV_VertexID`로 자기 격자 인덱스를 알아내 높이 버퍼(t39)를 직접 읽습니다.

```hlsl
float4 pos = float4(input.pos.x, g_vecCurrentHeightField[i], input.pos.z, 1.f);

float3 tangent   = normalize(float3(2 * g_fFluidDist, 0.f,
                    g_vecCurrentHeightField[i + 1] - g_vecCurrentHeightField[i - 1]));
float3 bitangent = normalize(float3(0.f, -2 * g_fFluidDist,
                    g_vecCurrentHeightField[i + g_iFluidWidth] - g_vecCurrentHeightField[i - g_iFluidWidth]));
float3 normal    = normalize(cross(tangent, bitangent));
```

노멀 텍스처를 쓰지 않고 이웃 두 점의 중앙차분으로 접선·종법선을 만들어 외적합니다. 파형이 바뀌면 노멀이 자동으로 따라옵니다.

**(5) 격자 생성 — `Fluid::CreateVertexBufferAndIndexBuffer()` (`Fluid.cpp:51`)**

`(i + j) % 2`로 사각형마다 대각선 방향을 번갈아 바꿉니다. 대각선을 한 방향으로만 깔면 파형이 그 방향으로 미세하게 쏠려 보입니다.

**(6) 입력 — `Fluid::Input()` (`Fluid.cpp:108`)**

스페이스바를 떼면 임의의 격자 칸 하나에 높이 15를 직접 써넣어 파문을 만듭니다.

### 한계

- **경계 처리가 좌우만 되어 있습니다.** 커널은 `x == 0` / `x == width-1`만 0으로 고정하고 상하 경계는 처리하지 않습니다. y=0에서 `indexdown`이 음수가 되어 버퍼 범위를 벗어납니다(D3D 구조적 버퍼의 범위 밖 읽기는 0을 반환하므로 크래시는 나지 않지만, 위아래 경계 거동은 좌우와 다릅니다).
- **정점 셰이더의 노멀 계산도 같은 문제를 갖습니다.** `i±1`, `i±width`를 범위 검사 없이 읽으므로 격자 가장자리의 노멀이 잘못됩니다.
- 높이장 시뮬레이션이라 물이 접히거나 튀는 것을 표현할 수 없습니다. 잔잔한 수면 전용입니다.
- 파문 입력이 디버그용 키 입력 하나뿐이고, 오브젝트가 물에 들어갈 때 파문을 만드는 상호작용이 없습니다.
- 굴절·반사가 없습니다. 물 재질은 일반 알파 셰이더(`AlphaNoUVPS`)를 그대로 씁니다. 수면다운 느낌을 내려면 화면 색을 노멀로 왜곡해 샘플링하는 굴절이 필요합니다.
- 200×200 격자 기준 정점 40,401개를 매 프레임 그리고, LOD나 화면 공간 테셀레이션이 없습니다.
- **종법선 계산의 축 배치가 접선과 다릅니다.** 접선은 `(2d, 0, Δh)`로 격자 간격을 x에, 높이차를 z에 놓는데, 종법선은 `(0, -2d, Δh)`로 격자 간격을 y에 놓습니다. 격자가 XZ 평면에 놓여 있으므로 종법선도 `(0, Δh, 2d)` 형태여야 맞아 보입니다. 결과 화면이 그럴듯해서 넘어간 부분이고, 다시 검토해야 합니다.
- 현재 저장소 상태에서는 씬에 생성되어 있지 않습니다. `Editor/Include/Scene/InGameScene.cpp`에 `#include "Bindable/Fluid.h"`(`:15`)만 남아 있고 생성 호출은 삭제된 상태입니다. 마지막으로 동작한 설정은 커밋 `a7c81ac`의 `InGameScene.cpp:203`, `(200, 200, d=0.1, mu=1.0, c=2.0)`입니다.

### 참조 위치

| 대상 | 위치 |
|---|---|
| 시뮬레이션 컴포넌트 | `Engine/Include/Bindable/Fluid.cpp` — 생성자 `:20`, `CreateVertexBufferAndIndexBuffer()` `:51`, `Input()` `:108`, `FixedUpdate()` `:121`, `Ready()` `:180` |
| 시뮬레이션 커널 | `Client/Bin/Resource/Shader/ComputeShader.hlsl` — `CS_FLUID` `:197` |
| 변위 정점 셰이더 | `Client/Bin/Resource/Shader/VertexShader.hlsl` — `VS_FLUID` `:66` |
| 고정 스텝 루프 | `Engine/Include/Core/Window.cpp:442` |
| 상수 버퍼 정의 | `Client/Bin/Resource/Shader/shared.hlsl` — `cbuffer Fluid` b11 `:319` |

---

## 부록 — 외부 코드 및 교과서 유래 표기

이 문서에서 "직접 작성"이라고 한 범위를 명확히 하기 위해, 저장소에 포함된 외부 코드와 교과서에서 가져온 부분을 정리합니다.

### 인용 범위

이 문서가 다루는 것은 커밋 `acddc8d`(2024-05-12)까지의 코드입니다. 그 이후 커밋(`7879a31` 이하 21개)은 AI 도구로 작성한 것이라 이 문서의 대상이 아닙니다.

### 외부 라이브러리 (저장소에 소스가 포함되어 있으나 작성한 것이 아님)

| 라이브러리 | 위치 | 용도 |
|---|---|---|
| Detour | `Engine/Include/Navigation/Detour/` | 경로 탐색, 군중 시뮬레이션(`DetourCrowd`) |
| Recast | `Editor/Include/Navigation/Recast/`, `.../DebugUtils/` | 내비게이션 메쉬 생성 및 디버그 드로우 |
| Dear ImGui | `Editor/Include/Imgui/` | 에디터 UI (내부 폰트 래스터라이저 `imstb_truetype.h` 포함) |
| FMOD | `Engine/Include/Sound/inc/` | 사운드 |
| DirectXTex | `Engine/Include/Bindable/DirectXTex.h` | 텍스처 로딩 |

### 교과서 유래 (구현은 직접 했으나 알고리즘·코드 구조가 공개 교재를 따름)

| 대상 | 위치 | 출처 |
|---|---|---|
| HDR 휘도 다운스케일 (`DownScale4x4` / `DownScale1024to4` / `DownScale4to1`), 톤매핑, `DistanceDOF`, 블룸 분리형 가우시안(`VerticalFilter` / `HorizonFilter`) | `HDR.fx` | Jason Zink 외, *Practical Rendering and Computation with Direct3D 11* 의 후처리 예제와 함수 구성·명명이 거의 동일 |
| 테셀레이션 포인트 라이트 볼륨 (`VS_PointLight` / `HS_PointLight` / `DS_PointLight`) | `anisotropic_microfacet.hlsl:574~642` | 같은 책의 디퍼드 조명 예제 |
| 유체 파동방정식 계수 `c1`/`c2`/`c3` 와 두 안정 조건 | `Fluid.cpp:20` | Eric Lengyel, *Mathematics for 3D Game Programming and Computer Graphics* 의 수면 절 |
| 이방성 Beckmann 분포(`:582`, `:587`), Cook-Torrance 기하 감쇠(`:595`), 굴절률 기반 프레넬(`:552`, `:569`) | `shared.hlsl:552~598` | 표준 마이크로패싯 BRDF 문헌 |
| FABRIK 반복 해법 | `Animation.cpp:726` | Aristidou & Lasenby, *FABRIK* 논문의 표준 알고리즘 |

위 항목들에서 GPU 이식(컴퓨트 셰이더화, 구조적 버퍼 설계, 버퍼 링), 엔진 리소스 배선, 렌더 패스 통합은 직접 작성한 부분입니다.

### 측정치에 대해

이 문서에는 성능 수치가 없습니다. 저장소에 프로파일링 결과나 벤치마크 로그가 남아 있지 않아, 측정하지 않은 값을 쓰지 않았습니다.
