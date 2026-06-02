/**
 * 게임 텔레메트리 수집용 Google Apps Script (doPost).
 * C++ 클라이언트가 보내는 run_start / run_end 이벤트를 Google Sheets 에 한 줄씩 적재한다.
 *
 * ── 배포 방법 ────────────────────────────────────────────────────────────────
 *  1) 이 코드를 받을 Google 스프레드시트를 하나 만든다.
 *  2) 시트 메뉴: 확장 프로그램(Extensions) > Apps Script 를 열고 이 파일 내용을 붙여넣는다.
 *  3) 아래 SECRET 값을 C++ 쪽 Telemetry.cpp 의 kSecret 과 "똑같이" 바꾼다.
 *  4) 배포(Deploy) > 새 배포(New deployment) > 유형: 웹 앱(Web app).
 *       - 설명: 아무거나
 *       - 실행 계정(Execute as): "나(Me)"  ← 내 권한으로 시트에 쓰기 위함
 *       - 액세스 권한(Who has access): "모든 사용자(Anyone)"  ← 익명 클라가 인증 없이 POST
 *  5) 배포하면 .../exec 로 끝나는 웹앱 URL 이 발급된다.
 *     이 URL 을 C++ 쪽 Telemetry.cpp 의 kEndpointUrl 에 넣는다.
 *  6) 코드를 수정하면 매번 "새 배포" 또는 기존 배포의 "버전 관리(Manage deployments)"에서
 *     새 버전으로 갱신해야 /exec 동작에 반영된다.
 *
 *  ※ /exec 는 보통 302 리다이렉트를 반환한다(정상). 클라이언트는 리다이렉트를
 *    따라가지 않고 302 를 성공으로 간주한다 — doPost 는 POST 수신 시점에 이미 실행된다.
 * ─────────────────────────────────────────────────────────────────────────────
 */

// 클라이언트(Telemetry.cpp kSecret)와 반드시 일치시킬 것.
var SECRET = 'tm_8f3kZ9xQ';

// 적재할 시트 이름(없으면 자동 생성).
var SHEET_NAME = 'events';

// 컬럼 순서. 클라 시계는 믿지 않고 server_time 은 서버에서 찍는다.
var HEADERS = [
  'server_time',        // 서버 수신 시각(new Date)
  'game_version',
  'session_id',
  'run_id',
  'event_type',         // run_start | run_end
  'survival_time_sec',  // run_end 전용
  'round_reached',      // run_end 전용 ★
  'enemies_killed',     // run_end 전용 ★
  'player_level',       // run_end 전용
  'weapons',            // run_end 전용 (콤마 결합)
  'exit_reason',        // run_end 전용 (death|quit|cleared)
  'active_time_sec'     // run_end 전용: 선택지/일시정지 제외 실제 플레이 시간(초)
];

function doPost(e) {
  try {
    // 1) 본문 파싱.
    if (!e || !e.postData || !e.postData.contents) {
      return jsonOut({ ok: false, error: 'no body' });
    }
    var data;
    try {
      data = JSON.parse(e.postData.contents);
    } catch (parseErr) {
      return jsonOut({ ok: false, error: 'bad json' });
    }

    // 2) secret 검증 — 불일치면 조용히 무시(노이즈/장난 차단).
    if (!data.secret || data.secret !== SECRET) {
      return jsonOut({ ok: false, error: 'unauthorized' });
    }

    // 3) 시트 확보(+ 헤더 1회).
    var sheet = getSheet_();

    // 4) weapons 가 배열이면 콤마로 결합.
    var weapons = data.weapons;
    if (Object.prototype.toString.call(weapons) === '[object Array]') {
      weapons = weapons.join(',');
    } else if (weapons == null) {
      weapons = '';
    }

    // 5) 스키마 순서대로 한 줄 append. 서버 시각을 직접 찍는다.
    var row = [
      new Date(),                       // server_time
      str_(data.game_version),
      str_(data.session_id),
      str_(data.run_id),
      str_(data.event_type),
      num_(data.survival_time_sec),
      num_(data.round_reached),
      num_(data.enemies_killed),
      num_(data.player_level),
      weapons,
      str_(data.exit_reason),
      num_(data.active_time_sec)
    ];
    sheet.appendRow(row);

    return jsonOut({ ok: true });
  } catch (err) {
    // 어떤 예외든 200/JSON 으로 마무리 — 클라는 어차피 응답을 신경 쓰지 않는다.
    return jsonOut({ ok: false, error: String(err) });
  }
}

// 시트를 가져오고, 비어 있으면 헤더 행을 1회 기록한다.
function getSheet_() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheetByName(SHEET_NAME);
  if (!sheet) {
    sheet = ss.insertSheet(SHEET_NAME);
  }
  if (sheet.getLastRow() === 0) {
    sheet.appendRow(HEADERS);
  }
  return sheet;
}

// run_end 가 아닌 이벤트(run_start)에는 없는 필드를 안전하게 비운다.
function str_(v) { return (v == null) ? '' : String(v); }
function num_(v) { return (v == null || v === '') ? '' : v; }

// ContentService 로 간단한 JSON 응답.
function jsonOut(obj) {
  return ContentService
    .createTextOutput(JSON.stringify(obj))
    .setMimeType(ContentService.MimeType.JSON);
}
