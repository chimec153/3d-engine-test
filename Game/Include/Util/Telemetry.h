#pragma once

// =====================================================================
//  Telemetry — 익명 플레이어 행동 텔레메트리 (WinHTTP HTTPS POST)
// ---------------------------------------------------------------------
//  목적: itch 공개 배포 후 "어디서 이탈하는지 / 몇 라운드까지 가는지 /
//        적을 얼마나 잡는지 / 어떤 무기를 고르는지"를 Google Sheets에 수집.
//
//  설계 요점:
//   - WinHTTP만 사용(윈도우 기본 내장). 외부 라이브러리/vcpkg 의존성 없음.
//   - 전송은 전부 백그라운드 워커 스레드 + 큐로 처리 → 게임 루프/렌더 절대 블로킹 X.
//   - 네트워크 단절/무응답이어도 조용히 무시(짧은 타임아웃). 게임은 멈추지 않음.
//   - Telemetry.cpp 한 곳에서만 싱글톤 인스턴스를 정의한다(헤더 inline static 금지).
//     Game.dll에 GAME_DLL로 export 되므로 Client.exe(main.cpp)와 Game.dll이
//     같은 단일 인스턴스를 공유한다. (inline 싱글톤은 모듈별로 따로 생기는 문제 회피)
// =====================================================================

#include <string>
#include <vector>

#include "Core/Macro.h"   // GAME_DLL (dllexport/dllimport)

namespace Client
{
    class GAME_DLL Telemetry
    {
    public:
        // Game.dll 한 곳에서만 정의되는 단일 인스턴스.
        static Telemetry& GetInst();

        // 프로그램 시작 시 1회. session_id(UUID) 발급 + 워커 스레드 기동.
        // 두 번 호출해도 안전(무시).
        void Init();

        // 프로그램 종료 시 1회. 큐에 남은 이벤트를 마저 보내고 워커를 조인.
        // 네트워크가 죽어 있어도 짧은 타임아웃 안에서 끝난다.
        void Shutdown();

        // 한 판(run) 시작. 새 run_id(UUID)를 발급하고 누적 카운터(처치 수,
        // 생존 시간 시작점)를 리셋한 뒤 "run_start" 이벤트를 큐에 넣는다.
        void RunStart();

        // 적 1기 처치 시 호출(스레드 안전). RunStart에서 0으로 리셋된다.
        void AddKill();

        // 한 판 종료. 누적된 enemies_killed/survival_time_sec를 자동으로 실어
        // "run_end" 이벤트를 큐에 넣는다. 같은 run_id로 두 번 보내지 않도록
        // 내부적으로 run_id를 소비(클리어)한다.
        //   exitReason : "death" | "quit" | "cleared"
        //   weapons    : 무기/업그레이드 식별 문자열(권장: 무기 ID를 문자열로).
        //                한글 이름을 넣을 경우 반드시 UTF-8이어야 한다(Utf8FromAcp 참고).
        //   activePlaySec : 선택지/일시정지 대기를 뺀 실제 플레이 시간(초).
        //                   survival_time_sec(벽시계)와 별도 필드로 함께 기록.
        void RunEnd(int                              iRoundReached,
                    int                              iPlayerLevel,
                    double                           dActivePlaySec,
                    const std::vector<std::string>&  vecWeapons,
                    const char*                      szExitReason);

        // 현재 판이 진행 중인지(run_id가 살아 있는지). RunEnd 중복 방지용으로
        // 호출부에서 가드할 때 사용.
        bool IsRunActive() const;

        // CP949(이 코드베이스의 소스/게임 문자열 인코딩) → UTF-8 변환 헬퍼.
        // 무기 "이름"처럼 한글이 섞인 문자열을 weapons에 넣고 싶을 때 사용.
        // (무기 ID 같은 ASCII 문자열은 변환 불필요.)
        static std::string Utf8FromAcp(const std::string& strAcp);

    private:
        Telemetry();
        ~Telemetry();
        Telemetry(const Telemetry&)            = delete;
        Telemetry& operator=(const Telemetry&) = delete;

        struct Impl;     // 스레드/큐/WinHTTP 세부는 .cpp에 은닉(pimpl)
        Impl* m_pImpl;
    };
}
