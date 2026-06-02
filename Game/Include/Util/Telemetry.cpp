// =====================================================================
//  Telemetry.cpp — WinHTTP 기반 비동기 텔레메트리 구현
// ---------------------------------------------------------------------
//  빌드 메모:
//   - winhttp.lib / rpcrt4.lib 는 아래 #pragma comment 로 링크한다(VS2022).
//     별도 프로젝트 설정 불필요. (원하면 프로젝트 속성 > 링커 > 입력 >
//     추가 종속성에 winhttp.lib;rpcrt4.lib 를 넣어도 동일하다.)
//   - 이 .cpp 는 unity(jumbo) 빌드에서 제외해 단독 TU 로 컴파일한다
//     (Game.vcxproj 에 <IncludeInUnityFile>false</IncludeInUnityFile>).
//     winhttp.h 매크로 충돌과, 이 코드베이스의 전역 `epsilon` 매크로가
//     STL 내부 헤더를 깨뜨리는 문제를 피하기 위함.
//   - 그래서 STL 스레드/네트워크 헤더를 Telemetry.h(=Core/Macro.h, epsilon
//     정의 포함)보다 "먼저" include 한다. include 순서를 바꾸지 말 것.
// =====================================================================

#include <winsock2.h>   // winhttp 보다 먼저(중복 windows.h 정의 충돌 방지)
#include <windows.h>
#include <winhttp.h>
#include <rpc.h>        // UuidCreate / UuidToStringA (rpcrt4)

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <cstdio>

#include "Telemetry.h"   // 같은 폴더. 반드시 STL 헤더들 "뒤"에 (epsilon 매크로 회피)

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "rpcrt4.lib")

// =====================================================================
//  설정값 — 배포 전 여기만 교체하면 된다.
// =====================================================================

// Apps Script 웹앱의 /exec URL (웹앱 배포 후 발급되는 주소).
static constexpr wchar_t kEndpointUrl[] =
    L"https://script.google.com/macros/s/AKfycbwqA24J1UvSHiGClTWFehvdkKdPHD7mRUl-0COwmSgh_-UQ0z3KDs-LhgHIVucnn6Fl/exec";

// Apps Script 쪽 doPost 의 SECRET 과 반드시 동일해야 한다(노이즈/장난 차단).
static constexpr char kSecret[] = "tm_8f3kZ9xQ";

// 빌드 버전 문자열. 릴리스마다 갱신.
static constexpr char kGameVersion[] = "0.1.0";

// 네트워크 타임아웃(ms). 짧게 잡아 끊긴 환경에서도 워커가 오래 매달리지 않게.
static constexpr int kTimeoutMs = 1500;

namespace Client
{
    // -----------------------------------------------------------------
    //  내부 헬퍼 — unity 빌드 익명 namespace 충돌을 피하려 named namespace.
    //  (이 TU 는 unity 제외라 사실 안전하지만 코드베이스 관례를 따른다.)
    // -----------------------------------------------------------------
    namespace telemetry_detail
    {
        void Log(const char* szMsg)
        {
            OutputDebugStringA("[Telemetry] ");
            OutputDebugStringA(szMsg);
            OutputDebugStringA("\n");
        }

        // 새 UUID 문자열 생성("xxxxxxxx-xxxx-...") . <random> 미사용(이 코드베이스의
        // epsilon 매크로가 <random>/<bit> 를 깨뜨림) — Win32 RPC API 로 생성.
        std::string MakeUuid()
        {
            UUID uuid{};
            if (UuidCreate(&uuid) != RPC_S_OK)
                return "00000000-0000-0000-0000-000000000000";

            RPC_CSTR szRaw = nullptr;
            if (UuidToStringA(&uuid, &szRaw) != RPC_S_OK || !szRaw)
                return "00000000-0000-0000-0000-000000000000";

            std::string strResult(reinterpret_cast<char*>(szRaw));
            RpcStringFreeA(&szRaw);
            return strResult;
        }

        // JSON 문자열 값 이스케이프(따옴표/역슬래시/제어문자). 입력은 UTF-8 가정.
        std::string JsonEscape(const std::string& strIn)
        {
            std::string strOut;
            strOut.reserve(strIn.size() + 8);
            for (unsigned char c : strIn)
            {
                switch (c)
                {
                case '\"': strOut += "\\\""; break;
                case '\\': strOut += "\\\\"; break;
                case '\n': strOut += "\\n";  break;
                case '\r': strOut += "\\r";  break;
                case '\t': strOut += "\\t";  break;
                default:
                    if (c < 0x20)
                    {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        strOut += buf;
                    }
                    else
                    {
                        strOut += static_cast<char>(c);
                    }
                    break;
                }
            }
            return strOut;
        }
    } // namespace telemetry_detail

    // =================================================================
    //  Impl — 스레드/큐/WinHTTP 세부 (pimpl)
    // =================================================================
    struct Telemetry::Impl
    {
        std::thread             worker;
        std::mutex              mtx;            // queue + stop 보호
        std::condition_variable cv;
        std::queue<std::string> queue;          // 전송 대기 중인 "완성된" JSON 본문
        std::atomic<bool>       bStop{ false };
        bool                    bInit = false;

        // 현재 판 상태(게임 스레드에서만 접근).
        std::string             strSessionId;
        std::string             strRunId;       // 비어 있으면 진행 중인 판 없음
        std::atomic<int>        iKills{ 0 };
        ULONGLONG               ullRunStartTick = 0;

        // 큐에 본문을 넣고 워커를 깨운다.
        void Enqueue(std::string strBody)
        {
            {
                std::lock_guard<std::mutex> lk(mtx);
                if (bStop) return;              // 종료 중이면 버린다
                queue.push(std::move(strBody));
            }
            cv.notify_one();
        }

        // 워커 루프: 큐가 빌 때까지 처리. stop && 큐 비면 종료.
        void WorkerLoop()
        {
            for (;;)
            {
                std::string strBody;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cv.wait(lk, [this] { return bStop || !queue.empty(); });

                    if (queue.empty())
                        return;                 // bStop == true 이고 더 보낼 것 없음

                    strBody = std::move(queue.front());
                    queue.pop();
                }
                // 네트워크 전송 — 실패는 조용히 무시(게임 흐름 무관).
                PostJson(strBody);
            }
        }

        // 단일 HTTPS POST. 성공 여부와 무관하게 절대 throw/block 하지 않음.
        void PostJson(const std::string& strBody)
        {
            // URL 파싱.
            URL_COMPONENTS uc{};
            uc.dwStructSize      = sizeof(uc);
            wchar_t szHost[256]  = {};
            wchar_t szPath[1024] = {};
            uc.lpszHostName      = szHost;
            uc.dwHostNameLength  = _countof(szHost);
            uc.lpszUrlPath       = szPath;
            uc.dwUrlPathLength   = _countof(szPath);

            if (!WinHttpCrackUrl(kEndpointUrl, 0, 0, &uc))
            {
                telemetry_detail::Log("URL 파싱 실패");
                return;
            }

            HINTERNET hSession = nullptr;
            HINTERNET hConnect = nullptr;
            HINTERNET hRequest = nullptr;

            hSession = WinHttpOpen(L"GameTelemetry/1.0",
                                   WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                   WINHTTP_NO_PROXY_NAME,
                                   WINHTTP_NO_PROXY_BYPASS, 0);
            if (hSession)
            {
                WinHttpSetTimeouts(hSession, kTimeoutMs, kTimeoutMs, kTimeoutMs, kTimeoutMs);

                hConnect = WinHttpConnect(hSession, szHost, uc.nPort, 0);
            }
            if (hConnect)
            {
                DWORD dwFlags = (uc.nScheme == INTERNET_SCHEME_HTTPS)
                                    ? WINHTTP_FLAG_SECURE : 0;
                hRequest = WinHttpOpenRequest(hConnect, L"POST", szPath,
                                              nullptr, WINHTTP_NO_REFERER,
                                              WINHTTP_DEFAULT_ACCEPT_TYPES, dwFlags);
            }
            if (hRequest)
            {
                // ★ 자동 리다이렉트 비활성화.
                //   Apps Script /exec 는 보통 302 로 응답한다. WinHTTP 가 302 를
                //   자동으로 따라가면 POST→GET 으로 바뀌며 본문이 유실된다.
                //   doPost 는 /exec 로 들어온 POST 시점에 이미 실행되므로,
                //   리다이렉트를 따라갈 필요가 없다. 따라가지 않음으로써 본문 유실을
                //   원천 차단하고, 302(3xx) 응답을 "전송 성공"으로 간주한다.
                DWORD dwDisable = WINHTTP_DISABLE_REDIRECTS;
                WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE,
                                 &dwDisable, sizeof(dwDisable));

                const wchar_t* szHeaders =
                    L"Content-Type: application/json; charset=utf-8\r\n";

                BOOL bSent = WinHttpSendRequest(
                    hRequest, szHeaders, (DWORD)-1L,
                    const_cast<char*>(strBody.data()),
                    static_cast<DWORD>(strBody.size()),
                    static_cast<DWORD>(strBody.size()), 0);

                if (bSent && WinHttpReceiveResponse(hRequest, nullptr))
                {
                    DWORD dwStatus = 0;
                    DWORD dwSize   = sizeof(dwStatus);
                    WinHttpQueryHeaders(
                        hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &dwStatus, &dwSize,
                        WINHTTP_NO_HEADER_INDEX);

                    // 200~399 = 서버 도달/처리 성공(302 포함). 그 외는 무시.
                    if (dwStatus < 200 || dwStatus >= 400)
                    {
                        char buf[64];
                        std::snprintf(buf, sizeof(buf), "전송 응답 코드 %lu", dwStatus);
                        telemetry_detail::Log(buf);
                    }
                }
                else
                {
                    telemetry_detail::Log("전송 실패(네트워크/타임아웃) - 무시");
                }
            }

            if (hRequest) WinHttpCloseHandle(hRequest);
            if (hConnect) WinHttpCloseHandle(hConnect);
            if (hSession) WinHttpCloseHandle(hSession);
        }
    };

    // =================================================================
    //  Telemetry — 공개 API
    // =================================================================

    Telemetry& Telemetry::GetInst()
    {
        // 함수 지역 static: 이 .cpp(=Game.dll) 한 곳에서만 생성되는 단일 인스턴스.
        static Telemetry inst;
        return inst;
    }

    Telemetry::Telemetry()  : m_pImpl(new Impl()) {}
    Telemetry::~Telemetry() { Shutdown(); delete m_pImpl; m_pImpl = nullptr; }

    void Telemetry::Init()
    {
        if (m_pImpl->bInit) return;

        m_pImpl->strSessionId = telemetry_detail::MakeUuid();
        m_pImpl->bStop.store(false);
        m_pImpl->bInit = true;
        m_pImpl->worker = std::thread([this] { m_pImpl->WorkerLoop(); });

        telemetry_detail::Log("초기화 완료 (session 시작)");
    }

    void Telemetry::Shutdown()
    {
        if (!m_pImpl || !m_pImpl->bInit) return;

        {
            std::lock_guard<std::mutex> lk(m_pImpl->mtx);
            m_pImpl->bStop.store(true);
        }
        m_pImpl->cv.notify_all();

        if (m_pImpl->worker.joinable())
            m_pImpl->worker.join();   // 큐에 남은 이벤트를 마저 보낸 뒤 종료

        m_pImpl->bInit = false;
        telemetry_detail::Log("종료");
    }

    bool Telemetry::IsRunActive() const
    {
        return !m_pImpl->strRunId.empty();
    }

    void Telemetry::AddKill()
    {
        m_pImpl->iKills.fetch_add(1, std::memory_order_relaxed);
    }

    // 공통 필드 + 추가 필드(extra)를 합쳐 JSON 본문을 만들고 큐에 넣는다.
    // extra 는 콤마로 시작하는 문자열(예: ,"round_reached":5,...) 또는 빈 문자열.
    // (이 합성 자체가 명세의 "SendEvent" — 게임 스레드에서 완성본을 만들어 큐로 넘긴다.)
    static std::string BuildBody(const std::string& strSession,
                                 const std::string& strRun,
                                 const char*        szEventType,
                                 const std::string& strExtra)
    {
        using telemetry_detail::JsonEscape;
        std::string b;
        b.reserve(256 + strExtra.size());
        b += "{";
        b += "\"secret\":\"";       b += JsonEscape(kSecret);       b += "\",";
        b += "\"game_version\":\""; b += JsonEscape(kGameVersion);  b += "\",";
        b += "\"session_id\":\"";   b += JsonEscape(strSession);    b += "\",";
        b += "\"run_id\":\"";       b += JsonEscape(strRun);        b += "\",";
        b += "\"event_type\":\"";   b += JsonEscape(szEventType);   b += "\"";
        b += strExtra;
        b += "}";
        return b;
    }

    void Telemetry::RunStart()
    {
        if (!m_pImpl->bInit) return;

        m_pImpl->strRunId = telemetry_detail::MakeUuid();
        m_pImpl->iKills.store(0);
        m_pImpl->ullRunStartTick = GetTickCount64();

        m_pImpl->Enqueue(BuildBody(m_pImpl->strSessionId, m_pImpl->strRunId,
                                   "run_start", ""));
    }

    void Telemetry::RunEnd(int                              iRoundReached,
                           int                              iPlayerLevel,
                           double                           dActivePlaySec,
                           const std::vector<std::string>&  vecWeapons,
                           const char*                      szExitReason)
    {
        if (!m_pImpl->bInit) return;
        if (m_pImpl->strRunId.empty()) return;   // RunStart 없이 호출 / 중복 방지

        const double dSurvivalSec =
            (GetTickCount64() - m_pImpl->ullRunStartTick) / 1000.0;
        const int    iKills = m_pImpl->iKills.load();

        // 추가 필드 합성.
        std::string strExtra;
        {
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                ",\"survival_time_sec\":%.1f,\"active_time_sec\":%.1f,"
                "\"round_reached\":%d,\"enemies_killed\":%d,\"player_level\":%d",
                dSurvivalSec, dActivePlaySec, iRoundReached, iKills, iPlayerLevel);
            strExtra = buf;
        }

        // weapons 배열.
        strExtra += ",\"weapons\":[";
        for (size_t i = 0; i < vecWeapons.size(); ++i)
        {
            if (i) strExtra += ",";
            strExtra += "\"";
            strExtra += telemetry_detail::JsonEscape(vecWeapons[i]);
            strExtra += "\"";
        }
        strExtra += "]";

        // exit_reason.
        strExtra += ",\"exit_reason\":\"";
        strExtra += telemetry_detail::JsonEscape(szExitReason ? szExitReason : "");
        strExtra += "\"";

        m_pImpl->Enqueue(BuildBody(m_pImpl->strSessionId, m_pImpl->strRunId,
                                   "run_end", strExtra));

        // run_id 소비 — 같은 판으로 run_end 가 두 번 나가지 않게.
        m_pImpl->strRunId.clear();
    }

    std::string Telemetry::Utf8FromAcp(const std::string& strAcp)
    {
        if (strAcp.empty()) return {};

        // CP949(ACP) → UTF-16 → UTF-8.
        int nWide = MultiByteToWideChar(CP_ACP, 0, strAcp.c_str(),
                                        (int)strAcp.size(), nullptr, 0);
        if (nWide <= 0) return {};

        std::wstring wbuf(nWide, L'\0');
        MultiByteToWideChar(CP_ACP, 0, strAcp.c_str(), (int)strAcp.size(),
                            &wbuf[0], nWide);

        int nUtf8 = WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), nWide,
                                        nullptr, 0, nullptr, nullptr);
        if (nUtf8 <= 0) return {};

        std::string out(nUtf8, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), nWide,
                            &out[0], nUtf8, nullptr, nullptr);
        return out;
    }
}
