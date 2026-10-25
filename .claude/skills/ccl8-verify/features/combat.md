# 최소 전투

서버가 행동 허용, 적중, 방어 결과, 능력치와 사망을 확정한다. Standalone, Dedicated, Listen에서 같은 규칙을 쓴다. 소유 문서는 `Docs/CombatFoundation.md`, 초기 수치의 근거는 결정 11이다.

## 하위 기능

- `attack`: 기본 공격의 적중과 스태미나 단일 차감
- `guard`: 방패 장착 시 정면 가드, 가드 붕괴와 경직, 후방 가드 실패
- `parry`: 성공 구간과 공격자 경직
- `dodge`: 회피 이동과 무적 구간, 비용 부족 거부
- `respawn`: 재스폰 후 ASC 유지, Avatar 교체, 자원 초기화
- `projectile`: 투사체 무기 발사와 적중

## 사용자 진입 경로

- `/Game/Maps/CombatPlayground`에서 좌클릭 공격, Q 패링, Shift 회피, R 재도전을 쓴다.
- 가드는 방패 픽업에 접근해 E로 얻고, I로 인벤토리를 열어 오른손에 장착한 뒤 우클릭을 유지한다. 빈손 우클릭은 공격이다.

## 실행

- 근접 전투 스모크(`Ability`): `.\Tools\Validation\run_combat_smoke.ps1 -Mode Standalone`, `-Mode Dedicated`, `-Mode Listen`, `-Mode Listen -DriverIsHost`. 지연·손실은 `-Impaired`를 더한다.
- 투사체 스모크(`internal`): `.\Tools\Validation\run_projectile_smoke.ps1`.
- 기대 결과: 출력 `PASS <Mode>`, 각 로그의 `CCL_COMBAT PASS`, 단계별 `CCL_COMBAT stage N passed`. 투사체는 `CCL_PROJECTILE PASS`.
- 증거: `Saved/Tests/CombatSmoke/`, `Saved/Tests/Projectile/`.
- 회귀: 전투 변경 뒤 `run_network_smoke.ps1`의 네 구성도 실행한다. 소유 문서가 이 회귀를 요구한다.
- MCP 경로: 입력 매핑, 방향 결정, 예고 가독성을 바꿨을 때 쓴다. 실제 클릭·키 입력 뒤 HP·스태미나·태그 변화와 화면을 기록한다.

## Gotchas

- 전투 스모크는 ASC에 Ability 입력 태그를 직접 보낸다. 마우스 버튼과 키가 올바른 태그로 매핑되는지는 확인하지 않는다.
- 소유 문서가 미검증으로 남긴 범위(방어 구간의 정확한 경계, 동시 피격, 서버 예측 거절, 재접속)는 스모크 PASS로 덮이지 않는다.
- 기본 CombatPlayground에 가드 시험용 장비가 빠졌던 이력이 있다(`Docs/FullGameplayMCPQA-2026-10-12.md`의 F-03). 가드 경로를 MCP로 볼 때는 방패 픽업이 있는지 먼저 확인한다.
