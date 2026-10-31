# 색인 도구

Graft와 Graphify의 사용, 재구성, 진단을 소유한다. 상위 문서는 [AgentContextTools](../AgentContextTools.md)다.

## Graft

스킬이 설명하는 `graft <인수>`는 이 프로젝트에서 `node Tools/Agent/graft.cjs cli <인수>`로 실행한다. 래퍼가 프로젝트 루트와 런타임을 고정하므로 전역 CLI가 없어도 된다.

```powershell
node Tools/Agent/graft.cjs cli map
node Tools/Agent/graft.cjs cli ask '심볼 또는 질문' --source
node Tools/Agent/graft.cjs engine --probe 'UObjectBase GetFName'
```

`graft/`는 이 저장소의 그래프 캐시다. 다른 프로젝트의 그래프를 복사하지 않는다. 그래프를 처음부터 다시 만들 때는 사용자 요청을 받고 `node Tools/Agent/graft.cjs cli build .`를 실행한다. Graft 0.18.0의 프로젝트 MCP는 그래프가 없으면 `tools/list`에 빈 목록을 돌려주고, 그래프를 만든 뒤 MCP를 다시 불러오면 도구가 나타난다.

엔진 인덱스는 `Tools/Graft/build_ue58_graft_indexes.cmd`가 로컬 경로 설정을 읽어 만든다. 큰 작업이므로 요청받은 범위에서만 실행하고, 외부 인덱스를 다른 작업이 쓰는지 먼저 확인한다.

`Tools/Graft/graft-orbis-local.patch`는 Graft 0.18.0의 파서 메모리, 대량 파일 열거, 확장자 필터를 보정한다. 이름과 내부 표식은 출처 확인용으로 보존했다. 실행 경로는 ccl8의 도구 사본과 로컬 런타임뿐이다. 라이선스는 `Tools/Graft/LICENSE`에 있다.

Graft 0.18.0은 캐시에 초기화 기록이 없으면 세션 시작·CLI·MCP 실행에서 전역 설정을 다시 쓸 수 있다. `configure_runtime.cjs`가 이 프로젝트 런타임에 관리 모드 가드를 적용하고, ccl8 진입점은 가드를 확인해 설정 재작성과 백그라운드 자동 갱신을 막는다. 가드가 없으면 진입점은 실행을 멈추므로 런타임을 다시 설치하면 가드도 다시 적용한다.

Claude 훅과 상태 표시는 같은 런타임 탐색기를 쓰고, 패키지가 없으면 원인을 stderr에 남긴 채 작업을 막지 않는다. 사용자 전역에도 같은 Graft 훅이 있으면 두 번 실행될 수 있다. 실제 중복이 확인되면 전역 훅 정리는 사용자와 별도로 진행한다.

## Graphify

사용자 설치의 `uv`, `graphify`, `claude`를 재사용한다.

```powershell
./Tools/Graphify/graphify_ccl8.cmd status
# 새 PC에서 설치와 스킬 복구가 필요할 때만:
./Tools/Graphify/graphify_ccl8.cmd setup
# 요청한 분석에 그래프가 필요할 때:
./Tools/Graphify/graphify_ccl8.cmd build canonical
./Tools/Graphify/graphify_ccl8.cmd query canonical '문서 간 관계 질문'
```

`canonical`은 운영·사양 문서, `research`는 Narrative의 Research·Drafts·Reviews 문서가 대상이다. 결과는 `.graphify-local/`에만 저장한다. `status`가 `missing`이나 `stale`을 보여도 정상이며 자동으로 빌드하거나 유료 호출을 하지 않는다. `setup`은 전역 도구 설치와 프로젝트 스킬 재생성을 포함한 복구 명령이므로 상태 확인에 쓰지 않는다.
