# Tripo 설치·SDK·CLI·플러그인 분석

Codex에서 제작을 지시하는 흐름에는 Tripo 3D 플러그인과 CLI를 먼저 검토한다. 자체 제작 도구를 개발할 때는 REST 또는 SDK가 적합하고, 반복적인 이미지 처리 그래프가 필요하면 ComfyUI가 대안이다. 아래 버전과 기능은 2026-11-18 공식 문서 기준이다. [조사 지도](../Tripo-2026-11-18.md)

## 실행 수단 비교

| 수단 | 작업 방식 | 필요한 준비 | ccl8에서의 판단 |
|---|---|---|---|
| Codex 플러그인 | 자연어 → 에이전트의 CLI 실행 → 프로젝트 파일 | 플러그인·Node·CLI·브라우저 인증 | 소량 에셋 제작의 첫 경로 |
| CLI | 명령·JSON·체인·배치 | Node 20+, 인증된 프로필 | 재현할 요청과 파일 기록을 남기기 쉬움 |
| REST/SDK | 비동기 API를 프로그램에서 제어 | API 키, 폴링·다운로드·오류 처리 | 전용 제작 도구가 필요할 때 |
| ComfyUI | MODEL_INFO로 처리 노드 연결 | ComfyUI, 노드 패키지, Python SDK | 이미지와 3D 처리 그래프를 반복할 때 |
| UE 플러그인 | Blueprint에서 생성·조회·다운로드 | 공식 패키지와 엔진 호환 검증 | 현재 문서가 UE 5.6.0만 명시하므로 별도 시험 |

이 표는 선택 제안이다. Codex 플러그인의 존재, CLI 설치, 계정 인증, UE 플러그인 설치는 서로 다른 준비 상태다.

## Codex와 로컬 확인

[Codex 안내](https://developers.tripo3d.ai/ko/docs/codex-plugin)는 자연어로 이미지·텍스트·기존 모델을 지정하면 플러그인이 생성하고 프로젝트에 저장하는 흐름을 설명한다. 일반 ChatGPT 대화와 Codex 실행 환경을 구분하며, 두 스킬은 일반 3D 제작과 게임 에셋 제작용이다.

브라우저 기기 인증에서는 URL과 일회용 코드를 사용자가 확인하고 승인한다. API 키를 채팅에 붙여 넣을 필요가 없다. 생성 결과는 보통 모델 파일, 서버가 제공하면 preview.png, 작업 정보 task.json으로 저장한다. 프리뷰가 마음에 들지 않아 다시 생성하면 새 작업 비용이 생긴다.

2026-11-18 로컬 읽기 결과는 다음과 같다.

| 검사 | 결과 | 해석 |
|---|---|---|
| 플러그인 매니페스트 | tripo-3d 0.2.7, skills 디렉터리 제공 | 설치 파일 존재 확인 |
| 현재 PATH의 tripo | Get-Command에서 찾지 못함 | 이 셸의 CLI 준비 완료를 확인할 수 없음 |
| Node | v24.21.0 | 문서의 20+ 조건 충족 |
| 로그인·잔액·실제 생성 | 실행 안 함 | 준비 완료 또는 생성 성공으로 보고하지 않음 |

매니페스트 위치는 로컬 플러그인 캐시의 `tripo-3d/0.2.7/.codex-plugin/plugin.json`이다. 설치·설정 정본은 [AgentContextTools](../../AgentContextTools.md)이며 이 조사는 설정을 바꾸지 않는다.

## CLI 0.9.0

[CLI 문서](https://developers.tripo3d.ai/ko/docs/cli)의 기준 버전은 0.9.0이다. 설치는 npm의 tripo-cli, 브라우저 로그인은 계정 사이트에 따라 ov 또는 cn을 선택한다. 언어만으로 리전을 추정하지 않는다. doctor는 환경·인증·네트워크·잔액을 검사하며 잔액 0은 설치 실패와 구분한다.

| 명령군 | 명령 | 기능 |
|---|---|---|
| 생성·재실행 | make, ai, redo | 입력 추론과 처리 체인, 계획, 새 seed 재실행 |
| 결과 확인 | view, history | GLB/GLTF 미리보기, 로컬 작업 기록 |
| 인증 | login, logout, whoami, use | 기기 인증·명명된 프로필 전환 |
| 계정 | topup, balance, usage | 결제 페이지 열기, 잔액, 비용 기록 |
| 명시적 API | generate, model, anim, mesh | 생성 엔드포인트·후처리 직접 지정 |
| 버전 목록 | models | 0.8.1+의 로컬 모델 ID·별칭 목록. API 호출 아님 |
| 작업·파일 | task get/list/watch, files upload | 상태 재개, 다운로드, 토큰 발급 |
| 배치 | batch run | manifest 기반 재개·동시 실행 |
| 관리 | config, doctor, update, docs | 설정·진단·버전·번들 명세 |
| 연동 | mcp, completion | 0.9의 --toolset full, Bash/Zsh/Fish 자동 완성 |

make는 입력을 받아 제출·대기·다운로드한다. `--dry-run`은 업로드와 작업 생성 없이 실행 계획과 가능한 비용 추정치를 보여 준다. `--model`은 자동 선택과 `-p model=...`보다 우선한다. 별칭 tripo-p2와 tripo-v3.1은 날짜가 붙은 API 버전으로 변환된다.

공통 실행 옵션은 out, no-wait, no-download, name, timeout, notify, 반복 가능한 param이다. 자동화에는 json/yes/quiet/no-open/profile을 사용한다. `@last`, `@2`, `@name`으로 로컬 작업을 참조할 수 있지만 재현 기록에는 실제 task_id를 남기는 방안을 제안한다.

7개 프리셋은 game-mobile/game-pc/film/print/ar-web/anim/toy다. 프리셋은 작업 구성을 편하게 하지만 실제 요구 조건과 같다고 가정하지 않는다. 특히 **0.9부터 PBR은 프리셋에서도 명시해야 한다.** `-p pbr=true`는 후속 texture 단계에도 적용된다.

```powershell
# Inspect a plan without submitting a task.
tripo make concept.png --model tripo-p2 -p pbr=true --dry-run

# Resume an existing task after a local timeout.
tripo task watch '<task-id>' --download

# Inspect the installed CLI contract.
tripo docs --topic commands/make
tripo docs --topic examples/game-asset
```

이는 문법 예시이며 이 조사에서 실행하지 않았다. PowerShell에서는 `'@last'`, `'texture,rig'`처럼 특수 토큰을 인용한다. JSON 파이프는 PowerShell 7을 우선하고, 5.1은 단계별 변수 전달 예제가 따로 있다. 자동 완성은 PowerShell 미지원이다.

make/watch는 완료를 기다리는 명령이다. 대부분의 일회성 json 출력은 최종 한 줄이고, watch의 진행은 stderr로 나간다. 로컬 대기 시간 초과는 서버 취소가 아니다. 생성 POST의 네트워크 오류·5xx 뒤에는 기존 작업과 동결 크레딧을 먼저 확인한다.

| 종료 코드 | 의미 |
|---|---|
| 0 | 성공 |
| 1 / 2 / 3 | 내부 오류 / 사용법·파라미터 / 인증 |
| 4 / 5 | 크레딧 부족 / 콘텐츠 거절 |
| 6 | 작업 실패 또는 대기 시간 초과. 상태 재확인 필요 |
| 7 / 8 / 9 | 네트워크 / 리소스 없음 / 제한 초과 |

주요 환경 변수는 TRIPO_API_KEY, TRIPO_PROFILE, TRIPO_REGION, TRIPO_API_BASE_URL, TRIPO_PLATFORM_BASE_URL, TRIPO_HOME이다. 업데이트는 TRIPO_NO_UPDATE_CHECK와 TRIPO_NPM_REGISTRY, 프록시는 HTTPS_PROXY/HTTP_PROXY/NO_PROXY로 제어한다. tripo ai는 TRIPO_LLM_BASE_URL/API_KEY/MODEL을 별도로 받는다.

공식 해외 API의 익명 사용 분석은 기본 활성이라고 안내한다. 메타데이터를 수집하며 프롬프트·파일 경로·입력 에셋·키·다운로드 URL은 제외한다고 설명한다. `TRIPO_ANALYTICS_ENABLED=0`으로 끌 수 있다. 본 조사에서는 환경 변수를 변경하지 않았다.

## SDK 5종

[SDK 문서](https://developers.tripo3d.ai/ko/docs/sdk)는 아래 고정 버전을 기준으로 지원 표를 제공한다. SDK 버전, API v3, 생성 모델 버전은 서로 독립적이다.

| 언어 | 문서의 고정 패키지 | 실행 요구 |
|---|---|---|
| JavaScript/TypeScript | @vastai/tripo-sdk 0.5.0 | Node 18+ |
| Python | tripo3d 1.0.0 | Python 3.8+ |
| Go | tripo-go-sdk v0.5.0 | Go 1.21+ |
| Rust | tripo3d-sdk 0.5.0 | Rust 1.88+ |
| Java | 공식 저장소의 고정 소스 스냅샷, Maven 0.1.0-SNAPSHOT | JDK 8+ |

공통 흐름은 클라이언트 구성 → 생성 → 완료 대기 → 최종 상태 검사 → 모델 다운로드다. Python은 model_version을 REST model로 매핑하며, IS_GLOBAL로 리전을 선택한다. Java는 TripoRegion.GLOBAL, JS/Go/Rust 예제는 /v3가 포함된 baseUrl을 사용한다. 서명된 결과 URL 다운로드에는 API 키 헤더를 붙이지 않는다.

| 기능 차이 | 지원 | 누락·주의 |
|---|---|---|
| 기본 3D·텍스처·변환·가져오기·분할·보완·리토폴로지·애니메이션·공통 조회 | 5종 모두 문서 표에 표시 | 실제 실행 검증을 뜻하지 않음 |
| text/image-to-image, edit-multiview, Smart UV | JS/Python/Go/Rust | Java 미지원 |
| SmartSegment | JS/Go/Rust | Python/Java 미지원 |
| image-to-depth | Python | 목차에 독립 API 명세 없음 |
| usage/history | 최신 변경 공지에 있음 | SDK 표의 account/usage와 같은 기능으로 가정하지 않음 |

Python 0.x는 v2를 사용한다. SDK 문서는 v2 종료일을 2026-10-31로 명시하므로 새 연동은 1.x를 선택한다. refine/stylize는 폐기되었고 Python은 요청 없이 오류를 낸다. Java에 남은 메서드가 서비스 지원을 보장하지 않는다.

## 편집기 플러그인

[플러그인 목록](https://developers.tripo3d.ai/ko/docs/plugins)의 패키지 표는 다음과 같다.

| 제품 | 패키지 버전 | 문서의 호환 버전 | 사용 위치 |
|---|---|---|---|
| Unity | 1.0.1 | 2022+ | Tools의 Tripo API Unity |
| Unreal Engine | 1.0 | 5.6.0 | Plugins/Tripo3D, Blueprint 노드 |
| Blender | 0.7.3 | 3.0+ | View3D Sidebar의 Tripo 3D |
| Godot | 0.02 | 4.4+ | addons/tripo-godot, Node3D |
| Cocos Creator | 1.0.0 | 3.8.6+ | Tripo Model Generator, assets/resources |
| ComfyUI 목록의 ZIP | 1.1.21 | 0.3.14 | 전용 문서의 2.0 안내와 충돌 |

설치는 각 편집기의 패키지·애드온 경로를 사용한다. API 키는 선택한 플러그인에 설정한다. UE 예제는 생성·상태 콜백·다운로드까지이며 런타임 표시에는 별도 glTFRuntime 의존성을 안내한다. ccl8의 UE 5.8.3에서 빌드·실행한 증거는 없다.

## ComfyUI 2.0의 노드와 연결

[전용 문서](https://developers.tripo3d.ai/ko/docs/comfyui-nodes)는 노드 패키지 2.0.0과 tripo3d 1.x를 요구한다. Manager, comfy-cli, 공식 소스 설치 중 하나를 선택한다. 1.x 패키지는 API v2 기반이므로 플러그인 목록의 구형 ZIP 안내와 구분한다.

API 키 우선순위는 노드 apikey → 환경 변수 → config.json이다. 첫 호출에서 해외·중국 잔액 조회로 리전을 결정한다. model_file은 다운로드 경로, MODEL_INFO는 task_id·키·출력 메타데이터다. 후속 Tripo 노드는 MODEL_INFO로 연결하고 외부 모델은 Import로 컨텍스트를 만든다.

| 노드 ID | 역할·필수 입력 | 주된 옵션·주의 |
|---|---|---|
| TripoAPIDraft | text/image/multiview 생성 | 기본 H3.1. prompt/image 조건부 필수. 모델별 실제 면 수 한도 적용 |
| TripoImportModel | 파일 또는 model_url 가져오기 | URL 우선. 출력은 MODEL_INFO만 |
| TripoTextureModel | model_info 텍스처 | quality, seed, pbr, delight, text/image/style 참조, bake, part_names |
| TripoMeshSegmentation | model_info 분할 | v1 기본. v2의 granularity/ref_image/connectivity |
| TripoMeshCompletion | 분할 결과 보완 | part_names, completion_mode |
| TripoSmartLowPoly | 면 수 감소 | P-v2/P-v3는 REST v2/v3 별칭. face_limit, quad, bake, smart_uv |
| TripoMeshUV | UV 재구성 | 80,000 등가 삼각형 이하. model_file, MODEL_INFO, uv_render |
| TripoAnimateRigNode | 자동 검사·리깅 | 기본 v3, rig_type=auto일 때 검사. 명시 체형이면 검사 생략 |
| TripoAnimateRetargetNode | 리깅 모델에 프리셋 적용 | 버전 조합 검사. export_with_geometry 기본 false |
| TripoConvertNode | 형식 출력 | format, 메시·재질·좌표·애니메이션 옵션. MODEL_INFO 미반환 |
| TripoRefineModel | 지원 중단 | 숨겨진 호환 노드. 실행 시 요청 없이 중단 |
| TripoStylizeModel | 지원 중단 | 기존 입력 유지. 실행 시 요청 없이 중단 |

Generate의 전체 옵션군은 mode/apikey/prompt/negative_prompt, image와 left/back/right, model_version, texture/pbr, 세 종류 seed, texture_quality/geometry_quality/texture_alignment, face_limit/quad/compress/generate_parts/smart_low_poly, auto_size/orientation/export_orientation, file_prefix/output_directory다. UI 범위가 face_limit 500,000까지여도 모든 모델이 이를 허용하는 것은 아니다. 생성 노드 quality 선택은 standard/detailed로 제한되어 REST 표와 범위가 다르다.

Import의 선택기는 PLY/3MF 등도 표시하지만 REST import의 보장 형식과 다르다. Smart UV에서 미리보기가 없으면 노드가 64×64 검은 이미지를 반환한다. 이를 유효한 UV 미리보기로 해석하지 않는다. Convert의 OPEN_EXR 옵션도 REST 표에는 없어 검증 대상으로 남긴다.

공식 PNG 예제 5개는 텍스트 생성, 이미지 생성, 멀티뷰, 텍스처, 리깅·내보내기 그래프를 담는다. 기본 저장 위치는 ComfyUI/output이며 Generate/Import의 접두사와 디렉터리가 후속 노드로 전달된다. 노드 누락은 패키지 위치·의존성·재시작, 출력 이동 실패는 디렉터리와 원본 파일, 리그 실패는 체형·버전·프리셋 조합부터 확인한다.
