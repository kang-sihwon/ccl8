# 원정 진행

`Campaign` 맵은 서쪽 마을, 중앙의 일반 적 2명, 동쪽 보스 구역으로 이뤄진다. 일반 적을 모두 처치하면 보스가 생성되고 보스를 쓰러뜨리면 승리한다. 진행은 서버가 GameState로 복제한다. 소유 문서는 `Docs/CampaignFoundation.md`, 콘텐츠는 `Docs/ContentFoundation.md`, 성장은 `Docs/ProgressionFoundation.md`다.

## 하위 기능

- `progress`: 일반 적 처치 집계, 보스 활성화 거부와 활성화, 승리
- `late-join`: 승리 이후 늦게 접속한 클라이언트도 같은 진행을 받음
- `respawn`: 플레이어 사망이 진행을 되돌리지 않음
- `content`: 처치 보상, NPC·상점·퀘스트 연결
- `progression`: 성장 포인트와 지속 효과의 저장
- `arrival`: 도입 연출의 UI·카메라 소유와 해제, 미니맵과 전체 지도

## 사용자 진입 경로

- `/Game/Maps/Campaign`을 열거나 시작 화면의 새 원정으로 들어간다.
- WASD·마우스로 이동하고 전투 입력은 `combat.md`를 따른다. Esc는 세션 메뉴를 연다.
- 도입 연출이 끝나면 미니맵이 보이고 전체 지도는 지도 키로 연다.

## 실행

- 진행 스모크(`Ability`): `.\Tools\Validation\run_campaign_smoke.ps1 -Mode Standalone`, `-Mode Dedicated`, `-Mode Listen`, `-Mode Listen -DriverIsHost`. 콘텐츠는 `-Content`, 성장은 `-Progression`, 지연·손실은 `-Impaired`, 화면 캡처는 `-Rendered`를 더한다.
- 도입·지도 스모크(`internal`): `.\Tools\Validation\run_integration_smoke.ps1`.
- 기대 결과: 서버 로그 `CCL_CAMPAIGN SERVER PASS`, 클라이언트마다 `CCL_CAMPAIGN CLIENT PASS`, 늦은 접속자 `CCL_CAMPAIGN CLIENT PASS role=late`. 옵션을 쓰면 `CCL_CONTENT REWARD PASS`, `CCL_PROGRESSION PERSISTENCE PASS`도 확인한다. 통합 스모크는 `CCL_INTEGRATION PASS`와 CHECK 9개다.
- 증거: `Saved/Tests/CampaignSmoke/`, `Saved/Tests/Integration/<시각>/editor.log`, 캡처 `Saved/Tests/Presentation/1280x720-{arrival,minimap,world-map}.png`.
- MCP 경로: 보급품 습득, 마을 복귀, 주민 대화 접근처럼 이동이 필요한 경로에 쓴다. 절차와 판정은 [MCP PIE 실행 검증](../references/mcp-pie.md)의 Campaign·저장 행을 따른다.

## Gotchas

- 진행 스모크는 서버가 적의 위치를 고정하고 AI를 멈춘 상태에서 처치한다. 일반 적·보스의 자연스러운 추적과 난이도는 확인하지 않는다.
- 통합 스모크는 지도 토글을 `PC->ToggleWorldMap()`으로 직접 호출한다. 지도 키 입력은 MCP 경로로만 확인된다.
- 통합 스모크를 언급하는 기능 문서는 없다. 결과는 이 스킬의 보고에 남긴다.
