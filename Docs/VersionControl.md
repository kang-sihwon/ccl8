# 버전 관리

ccl8은 독립 Git 저장소이고 기본 브랜치는 `main`, 원격은 GitHub의 `origin`이다. 엔진 소스는 저장소 밖에서 관리한다. 이 문서는 동기화, 커밋 주체, LFS 에셋, 기기별 경로의 제출 기준을 소유한다.

## 동기화

- 세션을 시작하면 `git status --short --branch`로 브랜치, upstream, 미커밋 변경을 확인한다.
- 작업 트리가 깨끗하고 upstream이 있으면 `git pull --ff-only`, `git lfs pull` 순으로 받는다. 미커밋 변경이나 분기가 있으면 상태를 보고하고 사용자 지시를 기다린다.
- 브랜치 전환, reset, stash 삭제, 강제 push는 사용자가 명시적으로 요청할 때 한다. Claude Code에서는 `Tools/Agent/guard-commands.cjs` 훅이 파괴적 Git·삭제 명령에 확인을 요청한다. Codex에는 이 훅이 없으므로 같은 확인을 직접 한다.
- 의도한 파일만 stage하고 `git diff --cached`로 검토한다.
- 커밋 제목은 영어 명령형 한 줄로 쓰고, 필요하면 본문에 이유와 검증을 적는다.
- 2026-10-25 에이전트 작업 체계를 재구성하기 전의 규칙 문서는 태그와 브랜치 `agent-setup-v1`(`1d125d5`)에 있다([결정 28](decisions/0028-agent-workflow-restructure.md)). 2026-10-27 기준 둘 다 `origin`에도 있다. 이름이 같아 Git이 `refname is ambiguous` 경고를 내므로 명령에는 `refs/tags/agent-setup-v1`이나 `refs/heads/agent-setup-v1`을 쓴다.

## 커밋 주체

AI는 자신이 바꾼 `Docs/`, `README.md`, `AGENTS.md`, `CLAUDE.md`, `GLOSSARY.md`와 `.claude/`, `.agents/`, `.codex/`의 공유 지침·스킬을 작업 묶음별로 커밋한다. push는 사용자가 연결을 승인한 원격의 upstream 브랜치에만 하고, 그런 브랜치가 없으면 커밋 뒤 보고한다.

게임 코드·설정·에셋과 도구 코드의 커밋은 사용자가 정한 작업 범위를 따른다. 학습 기록의 자동 갱신만으로는 커밋하거나 push하지 않는다.

## 에셋과 생성 파일

- 에셋은 에디터에서 저장한 뒤 변경을 확인한다. `.uasset`, `.umap`, 원본 미디어는 `.gitattributes`에 따라 LFS로 관리한다.
- 같은 바이너리 에셋을 여러 기기에서 동시에 편집하지 않는다. 충돌하면 자동 병합을 기대하지 않고 보존할 버전을 사용자와 정한다.
- 새 PC는 `git lfs install --local`과 `git lfs pull`을 실행한다. 업로드 전에 `git lfs status`로 대상과 용량을 확인한다. 요금과 한도는 GitHub 계정 설정에서 확인한다.
- 엔진, 빌드 산출물, IDE 생성물, 로컬 설정, 분석 캐시는 추적하지 않는다. `Build/`의 아이콘·배포 원본과 필요한 `*_BuiltData.uasset`은 추적한다.

## 기기별 경로와 로컬 설정

저장소, 엔진, Graft 인덱스·런타임, 개인 볼트, 사용자 설치 폴더의 실제 경로는 기기마다 다르므로 stage·commit·push하지 않는다. 공유 문서와 설정은 다른 기기에서도 그대로 쓸 수 있어야 한다. 토큰과 플러그인 로그인도 공유 설정에 넣지 않는다.

- 실제 경로는 Git에서 제외된 `.local/agent-paths.json`, `.codex/config.toml`, `.claude/settings.local.json`에 둔다. 파일별 용도와 생성 방법은 [AgentContextTools](AgentContextTools.md)가 소유한다.
- 공유 설정·스크립트·훅은 저장소 상대 경로, 실행 파일 위치, 환경 변수로 경로를 구한다. 문서와 예시에는 `<repository>`, `<Engine>`, `<IndexRoot>`, `<VaultRoot>` 같은 자리표시자를 쓴다.
- 진단 출력, 로그, 완료 기록을 공유 문서에 옮길 때 실제 경로를 상대 경로나 자리표시자로 바꾸고 바꿨다는 사실을 적는다. 원본 로그는 로컬에 둔다.
- 새 기기 설정이나 폴더 이동으로 바뀐 로컬 파일은 커밋 대상이 아니다. 위 커밋 주체 규칙은 로컬 설정의 제출 권한을 포함하지 않는다.
- 커밋 전에 `git diff --cached --name-only`로 제출 파일을, `git diff --cached`로 기기별 경로가 없는지 확인한다. 로컬 설정은 `git check-ignore`와 `git ls-files`로 제외·미추적 상태를 확인하고, `git add -f`로 추가하지 않는다.
- `.gitignore`는 이미 추적 중인 파일을 제외하지 않는다. 추적 파일에서 실제 경로를 발견하면 그 파일의 제출을 멈추고, 원본을 로컬에 보존한 뒤 기기별 값을 로컬 설정으로 옮긴다. 과거 Git 이력은 다시 쓰지 않는다.
- 과거 인용과 기록은 고치지 않는다. 경로가 들어 있는 기록은 새로 제출하지 않는다.
