# 시작·접속·저장

시작 화면은 혼자 시작, 방 생성, 주소로 접속, 저장 불러오기와 종료를 제공한다. 게임 중 Esc는 같은 메뉴로 저장·설정·시작 화면 복귀를 연다. 소유 문서는 `Docs/SessionFoundation.md`다.

## 하위 기능

- `save-load`: 저장, 별도 프로세스 재개, 메뉴 복귀 뒤 재불러오기, 승리 상태 복원
- `host-join`: 방 생성과 주소 접속, 손님의 호스트 저장 요청 거부
- `bad-join`: 잘못된 주소와 없는 포트의 접속 실패 표시
- `reject`: 손상된 슬롯, 모르는 버전·정의, 음수 수량, 중복 GUID, 없는 장비 참조 거부
- `loadout-restore`: 여러 PlayerState의 효과 준비와 세계 복원 묶음, 실패 시 기존 상태 유지

## 사용자 진입 경로

- 프로젝트 기본 맵의 시작 화면에서 버튼을 마우스나 키보드로 고른다.
- 게임 중 Esc로 메뉴를 열고 저장·설정·시작 화면 복귀를 고른다.
- 접속은 메뉴의 주소 입력에 호스트 이름 또는 IPv4와 선택적 포트를 넣는다.

## 실행

- 세션 스모크(`Slate`):
  - 기본 실행은 write와 read를 별도 프로세스로 수행한다: `.\Tools\Validation\run_session_smoke.ps1`
  - 접속: `-Network`. 접속 실패: `-Failure`. 화면 캡처: `-Rendered`.
- Loadout 복원 스모크(`internal`): `.\Tools\Validation\run_loadout_restore.ps1`. 기본 맵은 EnvironmentScenario이고 `-Map EnvironmentPlayground`도 받는다.
- 기대 결과: 출력 `PASS Session network=<bool> failure=<bool> rendered=<bool>`, 역할별 로그의 `CCL_SESSION_TEST PASS write|read|host|join|badjoin`. Loadout은 `CCL_LOADOUT_RESTORE PASS`.
- 증거: `Saved/Tests/SessionSmoke/<시각>/<역할>.log`, `Saved/Tests/LoadoutRestore/`.
- MCP 경로: 전체 버튼의 마우스 조작, 화면 모드 전환, 실제 호스트 생성과 주소 접속의 시간 초과처럼 소유 문서가 미확인으로 남긴 범위에 쓴다.

## Gotchas

- 세션 스모크는 고정 포트 18791을 쓴다. 다른 세션 스모크와 동시에 실행하지 않는다.
- 테스트는 `CCL_Validation` 슬롯을 쓴다. 일반 플레이의 `CCL_Checkpoint` 슬롯과 혼동하지 않는다.
- 세션 스모크의 Slate 입력은 첫 버튼에 Enter를 보내는 경로다. 마우스로 버튼을 고르는 경로는 확인하지 않는다.
