# Tripo 원문 대조표

2026년 11월 18일 한국어 개발자 문서의 사이드바와 본문 링크를 대조했다. 중복 링크를 합치면 44개 페이지다. 같은 API를 설명해도 H와 P는 지원 옵션이 달라 각각 한 페이지로 셌다. 문서 시작 주소와 소개 페이지, 같은 항목의 Markdown 주소는 한 번만 셌다. 가격표는 요금 대조에 사용한 별도 출처다. [공식 목차](https://developers.tripo3d.ai/ko/docs) · [공식 가격표](https://developers.tripo3d.ai/ko/pricing) · [종합 분석](../Tripo-2026-11-18.md)

## 원문별 확인 내용

| 번호 | 공식 문서 | 분석한 내용 | 상세 분석 |
|---:|---|---|---|
| 1 | [Tripo API 소개](https://developers.tripo3d.ai/ko/docs/introduction) | v3의 공통 응답과 비동기 작업 생성·조회 흐름을 정의한다. | [운영](Operations.md) |
| 2 | [빠른 시작](https://developers.tripo3d.ai/ko/docs/quick-start) | 시나리오 6개와 기능 예제 8개에서 작업 순서, 모델 선택, 비용 예시를 대조했다. | [제작](ProductionWorkflow.md) |
| 3 | [인증](https://developers.tripo3d.ai/ko/docs/authentication) | Bearer API 키가 필요한 요청과 키 보관 방식을 확인했다. | [운영](Operations.md) |
| 4 | [변경 내역](https://developers.tripo3d.ai/ko/docs/changelog) | 날짜별로 텍스처 기본값, 정산 표시, 입력 형식, 리그·리토폴로지 변경을 추적했다. | [운영](Operations.md) |
| 5 | [SDK 연동](https://developers.tripo3d.ai/ko/docs/sdk) | 언어별 SDK 다섯 종류의 설치·작업 대기·파일 회수 지원 차이를 비교했다. | [도구](Tools.md) |
| 6 | [Tripo 플러그인](https://developers.tripo3d.ai/ko/docs/plugins) | 편집기별 패키지와 공개 호환 버전을 확인했다. | [도구](Tools.md) |
| 7 | [ComfyUI 노드](https://developers.tripo3d.ai/ko/docs/comfyui-nodes) | 노드 12개의 입출력, 기본값, 리그·생성 모델 제약을 조사했다. | [도구](Tools.md) |
| 8 | [Tripo CLI: AI로 텍스트와 이미지를 3D 모델로 만들기](https://developers.tripo3d.ai/ko/docs/cli) | 모델 별칭, dry-run, 재개, 출력 형식, 인증과 오류 처리를 정리했다. | [도구](Tools.md) |
| 9 | [Codex에서 Tripo 사용하기](https://developers.tripo3d.ai/ko/docs/codex-plugin) | Codex에서 파일을 지정하는 방법과 CLI 설치·계정 연결의 관계를 확인했다. | [도구](Tools.md) |
| 10 | [작업 수명주기](https://developers.tripo3d.ai/ko/docs/task-lifecycle) | 생성 응답의 작업 ID부터 종료 상태와 결과 파일까지 추적했다. | [운영](Operations.md) |
| 11 | [모델 및 버전](https://developers.tripo3d.ai/ko/docs/models-and-versions) | H/P 지오메트리 버전과 텍스처·리그 등 별도 버전의 용도를 구분했다. | [생성](Generation.md) |
| 12 | [청구](https://developers.tripo3d.ai/ko/docs/billing) | 가용 잔액, 작업 중 보류액, 성공·실패 정산의 관계를 정리했다. | [운영](Operations.md) |
| 13 | [3D 모델에 대한 텍스트](https://developers.tripo3d.ai/ko/docs/generation-text-to-model/standard) | H 텍스트 입력의 모델 버전, 세부 묘사, 텍스처 옵션을 확인했다. | [생성](Generation.md) |
| 14 | [텍스트를 3D 모델로 · P 시리즈](https://developers.tripo3d.ai/ko/docs/generation-text-to-model/p) | P 텍스트 입력에서 P1/P2의 면 수와 토폴로지 제약을 비교했다. | [생성](Generation.md) |
| 15 | [이미지를 3D 모델로](https://developers.tripo3d.ai/ko/docs/generation-image-to-model/standard) | H 단일 이미지의 파일 입력과 정렬·재질·출력 방향 조건을 확인했다. | [생성](Generation.md) |
| 16 | [이미지를 3D 모델로 · P 시리즈](https://developers.tripo3d.ai/ko/docs/generation-image-to-model/p) | 같은 이미지에서 P2의 면 수와 쿼드 출력이 결과 형식에 미치는 영향을 정리했다. | [생성](Generation.md) |
| 17 | [멀티뷰에서 3D 모델로](https://developers.tripo3d.ai/ko/docs/generation-multiview-to-model/standard) | H 멀티뷰의 직접 입력, 생성 작업 연결, 편집 뷰 조합을 비교했다. | [생성](Generation.md) |
| 18 | [멀티뷰에서 3D 모델로 · P 시리즈](https://developers.tripo3d.ai/ko/docs/generation-multiview-to-model/p) | P 멀티뷰가 공유하는 뷰 순서와 시리즈별 형상 옵션을 확인했다. | [생성](Generation.md) |
| 19 | [Gaussian Splat에 대한 이미지](https://developers.tripo3d.ai/ko/docs/generation-image-to-splat) | 단일 이미지에서 .splat을 출력하는 조건과 일반 메시와의 용도 차이를 정리했다. | [생성](Generation.md) |
| 20 | [텍스트를 이미지로](https://developers.tripo3d.ai/ko/docs/generation-text-to-image) | 지원 이미지 모델, 생성 옵션, 사용 종료 일정과 예제의 버전 표기를 대조했다. | [생성](Generation.md) |
| 21 | [이미지를 이미지로](https://developers.tripo3d.ai/ko/docs/generation-image-to-image) | 입력 이미지 편집과 자세·완성·추출용 템플릿을 비교했다. | [생성](Generation.md) |
| 22 | [이미지를 멀티뷰로](https://developers.tripo3d.ai/ko/docs/generation-image-to-multiview) | 정면 기준 이미지에서 여러 방향을 만들고 후속 작업에 넘기는 방식을 확인했다. | [생성](Generation.md) |
| 23 | [멀티뷰 편집](https://developers.tripo3d.ai/ko/docs/generation-edit-multiview) | 기존 멀티뷰에서 필요한 시점만 수정하는 입력·과금 단위를 확인했다. | [생성](Generation.md) |
| 24 | [모델 리텍스쳐링](https://developers.tripo3d.ai/ko/docs/models-texture) | 기존 형상의 텍스처 버전과 조명 제거, PBR 및 입력 경로를 정리했다. | [후처리](Processing.md) |
| 25 | [메시 의미론적 분할](https://developers.tripo3d.ai/ko/docs/mesh-segment) | 기존 메시를 의미 단위로 나눈 결과와 부품별 식별자를 확인했다. | [후처리](Processing.md) |
| 26 | [스마트 세분화](https://developers.tripo3d.ai/ko/docs/mesh-smartsegment) | 이미지와 GLB의 입력 조건, 변환 행렬, 작업 ID 재사용 제약을 정리했다. | [후처리](Processing.md) |
| 27 | [메시 부품 완성](https://developers.tripo3d.ai/ko/docs/mesh-complete) | 분할한 부품의 가려진 면을 보완하는 방식과 빠른 처리 대안을 비교했다. | [후처리](Processing.md) |
| 28 | [메시 데시메이션](https://developers.tripo3d.ai/ko/docs/mesh-decimate) | 리토폴로지 버전, 면 수, 쿼드와 내장 UV 선택 조건을 정리했다. | [후처리](Processing.md) |
| 29 | [스마트 UV](https://developers.tripo3d.ai/ko/docs/mesh-uv) | 독립 UV 작업을 리토폴로지의 UV 옵션과 비교했다. | [후처리](Processing.md) |
| 30 | [모델 형식 변환](https://developers.tripo3d.ai/ko/docs/models-convert) | 지원 출력 형식과 축·단위·피벗·텍스처 옵션, 분리 텍스처 FBX를 정리했다. | [후처리](Processing.md) |
| 31 | [강성 검사](https://developers.tripo3d.ai/ko/docs/animations-rig-check) | 리깅 가능 여부·체형 판정과 후속 리깅에 넘길 모델 식별자를 구분했다. | [애니메이션](Animation.md) |
| 32 | [자동 리깅](https://developers.tripo3d.ai/ko/docs/animations-rig) | 체형별 리그 버전, 스켈레톤 규격과 파일 입력 제약을 확인했다. | [애니메이션](Animation.md) |
| 33 | [애니메이션 리타겟팅](https://developers.tripo3d.ai/ko/docs/animations-retarget) | 버전별 프리셋, 리그 유지 조건과 애니메이션 내보내기 옵션을 비교했다. | [애니메이션](Animation.md) |
| 34 | [작업 쿼리](https://developers.tripo3d.ai/ko/docs/task-query) | 진행률, 성공 파일, 실패 코드, 소비 크레딧을 조회하는 단위다. | [운영](Operations.md) |
| 35 | [일괄 작업 쿼리](https://developers.tripo3d.ai/ko/docs/task-batch-query) | 여러 작업의 조회 한도와 응답 형태를 확인했다. | [운영](Operations.md) |
| 36 | [파일 관리](https://developers.tripo3d.ai/ko/docs/files) | 멀티파트 업로드의 허용 형식, 용량, 파일 토큰의 용도를 정리했다. | [운영](Operations.md) |
| 37 | [파일 업로드(대용량 파일)](https://developers.tripo3d.ai/ko/docs/files-presign) | 업로드 주소 발급, 직접 전송, 파일 토큰 사용 순서를 확인했다. | [운영](Operations.md) |
| 38 | [계정 관리](https://developers.tripo3d.ai/ko/docs/account) | 잔액·보류액과 작업별 사용 내역을 조회하는 두 경로를 확인했다. | [운영](Operations.md) |
| 39 | [사용 내역](https://developers.tripo3d.ai/ko/docs/account-usage-history) | 기간별 사용 내역과 페이지 순회로 비용을 대조하는 방법을 정리했다. | [운영](Operations.md) |
| 40 | [오류 처리](https://developers.tripo3d.ai/ko/docs/error-handling) | 요청 단계 오류와 처리 중 실패를 구분해 재시도 판단을 정리했다. | [운영](Operations.md) |
| 41 | [속도 제한 및 동시성](https://developers.tripo3d.ai/ko/docs/rate-limits) | 키별 요청 속도와 계정별 동시 작업 한도, 대기 헤더를 구분했다. | [운영](Operations.md) |
| 42 | [Webhooks](https://developers.tripo3d.ai/ko/docs/webhooks) | 이벤트 세 종류, 서명 검증, 중복 전달 판별과 수신 처리 순서를 정리했다. | [운영](Operations.md) |
| 43 | [v2에서 v3로 마이그레이션 가이드](https://developers.tripo3d.ai/ko/docs/migration-v2-to-v3) | 범용 작업 유형에서 기능별 경로로 바뀐 점, 필드 이름과 폐기 기능을 대조했다. | [운영](Operations.md) |
| 44 | [모델 가져오기](https://developers.tripo3d.ai/ko/docs/models-import) | 외부 모델을 가져올 때의 파일 형식, 좌표 정규화와 원래 변환 복원 정보를 확인했다. | [후처리](Processing.md) |

## 빠른 시작의 선택 탭

각 탭의 흐름과 예제에서 선택한 옵션을 확인했다. 예제의 면 수·소요 시간·비용은 해당 조합의 안내값이다. 실제 예산은 선택한 모델과 후처리별 단가로 계산한다. [빠른 시작](https://developers.tripo3d.ai/ko/docs/quick-start) · [요금 분석](Operations.md#크레딧과-요금)

| 시나리오 | 예제에서 확인한 선택 | 적용할 때 볼 점 |
|---|---|---|
| 게임 캐릭터 | P2 이미지, 5,000면 → 검사 → v3 이족 리그 → 걷기 | 검사에 사용한 원본 모델을 리깅에 넘긴다 |
| 고해상도 모델 | H3.1 텍스트, 형상·재질 상세도와 PBR 지정 | 비슷한 외형이라도 재질 옵션에 따라 비용이 다르다 |
| 3D 프린팅 | H3.1에서 재질 없이 형상 생성 → STL | 3MF 색상 설명은 변환 레퍼런스와 대조한다 |
| 애니메이션 장면 | H3.1 → 검사 → v3 이족 리그 → 걷기 | 동물의 리그와 프리셋은 별도로 선택한다 |
| 상품 시각화 | 단일 제품 이미지 → H3.1 PBR → GLTF | 그림자·배경·가려진 면을 확인한다 |
| 멀티뷰 복원 | 정면 이미지 → 멀티뷰 생성 → H3.1 | 생성 뷰를 검토한 뒤 작업 ID를 전달한다 |

| 기능 예제 | 모델 제작에 주는 의미 |
|---|---|
| 형상만 생성 | P2에서 texture와 pbr을 끄고 면 수 목표를 준다. 낮은 값에서의 품질 권고와 API 입력 한도는 구분한다 |
| 텍스처 포함 생성 | H3.1에서 형상과 표준 텍스처를 함께 요청한다 |
| PBR 생성 | 고상세 형상과 텍스처, PBR을 함께 켠 조합이다 |
| 재텍스처 | 기존 모델의 형상을 유지하며 텍스트·이미지 지시로 재질을 바꾸는 예시다 |
| 리그 추가 | 검사 결과의 체형을 바탕으로 버전을 고른다. 구형 이족 리그는 재리깅이 필요할 수 있다 |
| 애니메이션 추가 | 리깅 결과를 retarget에 연결해 프리셋을 적용한다 |
| 파일 형식 변환 | 같은 입력을 FBX·USDZ·OBJ 등으로 내보낸다. 변환 옵션에 따라 형상과 가격도 달라진다 |
| 멀티뷰 복원 | 한 장에서 생성한 시점들을 묶어 재구성한다. 시점 간 일치 여부를 사람이 먼저 볼 수 있다 |

가격표에서 H·P·스플랫 탭을 각각 확인했고, 재텍스처·메시 처리·리깅·이미지 생성 단가도 대조했다. 리타겟 페이지는 펼친 프리셋 목록까지 검토했다. [가격표](https://developers.tripo3d.ai/ko/pricing) · [프리셋 분석](Animation.md#프리셋-목록)

## 문서 불일치와 대응

서로 다른 페이지의 안내가 실제 선택을 바꾸는 사례다. 실행 결과가 필요한 항목에는 판단 근거와 확인 방법을 함께 적었다.

| 항목 | 원문에서 다른 점 | 적용 기준 |
|---|---|---|
| ComfyUI 버전 | 플러그인 개요에는 1.1.21, 전용 문서에는 2.0.0이 적혀 있다. [원문 6](https://developers.tripo3d.ai/ko/docs/plugins) · [원문 7](https://developers.tripo3d.ai/ko/docs/comfyui-nodes) | 전용 문서와 실제 설치 버전을 기준으로 노드 입력을 확인한다. |
| PBR 기본값 | REST 생성 표는 기본 활성화, CLI 0.9는 기본 비활성화로 설명한다. [원문 15](https://developers.tripo3d.ai/ko/docs/generation-image-to-model/standard) · [원문 8](https://developers.tripo3d.ai/ko/docs/cli) | 필요하면 CLI에 `-p pbr=true`를 지정하고 계획을 확인한다. |
| 텍스처 버전 | 9월 출시 안내와 11월 기본값 변경 시점이 다르다. v3.0 호환 처리에도 delight 지정 여부가 관여한다. [원문 4](https://developers.tripo3d.ai/ko/docs/changelog) · [원문 24](https://developers.tripo3d.ai/ko/docs/models-texture) | 재현할 작업에는 원하는 버전과 delight를 모두 지정한다. |
| 이미지 모델 이름 | 예제의 Seedream 철자와 일부 예제의 버전이 지원값 표와 다르다. [원문 20](https://developers.tripo3d.ai/ko/docs/generation-text-to-image) · [원문 21](https://developers.tripo3d.ai/ko/docs/generation-image-to-image) | 해당 엔드포인트 표에 있는 식별자와 지원 종료일을 확인한다. |
| 이미지 배경·출력 | 변경 공지와 본문은 해상도·투명 PNG를 언급하지만 매개변수 표에 일부 필드가 빠져 있다. [원문 4](https://developers.tripo3d.ai/ko/docs/changelog) · [원문 20](https://developers.tripo3d.ai/ko/docs/generation-text-to-image) | 구현할 때 설치 SDK 명세로 필드 이름과 허용값을 확인한다. |
| 리그·프리셋 호환 | 리타겟 표의 버전별 프리셋 범위보다 ComfyUI 노드가 제한하는 체형·버전 조합이 좁다. [원문 33](https://developers.tripo3d.ai/ko/docs/animations-retarget) · [원문 7](https://developers.tripo3d.ai/ko/docs/comfyui-nodes) | 이족 v3, 사족 v2.5부터 해당 규격의 프리셋으로 시험한다. |
| 리그의 외부 파일 | 리그 입력 표와 변경 공지에서 URL 및 허용 확장자의 설명 범위가 다르다. [원문 32](https://developers.tripo3d.ai/ko/docs/animations-rig) · [원문 4](https://developers.tripo3d.ai/ko/docs/changelog) | 가져오기 작업 ID 또는 문서에 명시된 파일 토큰부터 검증한다. |
| 리텍스처 지시 | 예제는 `texture_prompt`를 쓰지만 REST 요청 필드 표에서 해당 구조가 빠져 있다. [원문 2](https://developers.tripo3d.ai/ko/docs/quick-start) · [원문 24](https://developers.tripo3d.ai/ko/docs/models-texture) | SDK 직렬화 결과와 실제 요청으로 스키마를 검증한다. |
| 리타겟의 입력 모델 | 빠른 시작의 제한과 달리 레퍼런스에는 리그를 유지한 후처리 모델을 받는 경로가 있다. [원문 2](https://developers.tripo3d.ai/ko/docs/quick-start) · [원문 33](https://developers.tripo3d.ai/ko/docs/animations-retarget) | 최초 시험에는 리그 작업 결과를 쓰고 후처리 뒤 스켈레톤 보존 여부를 점검한다. |
| 업로드와 가져오기 | multipart와 presign의 WebP 안내가 다르다. 업로드 허용 형식과 가져오기 허용 형식도 다르다. [원문 36](https://developers.tripo3d.ai/ko/docs/files) · [원문 37](https://developers.tripo3d.ai/ko/docs/files-presign) | 실제 사용할 업로드 방법과 최종 작업 양쪽에서 같은 형식을 지원하는지 본다. |
| 멀티뷰 입력 수 | ComfyUI 포트는 정면 이외가 선택 사항으로 보이지만 REST 직접 이미지 입력은 정면 포함 최소 두 시점을 요구한다. [원문 7](https://developers.tripo3d.ai/ko/docs/comfyui-nodes) · [원문 17](https://developers.tripo3d.ai/ko/docs/generation-multiview-to-model/standard) | 시점 파일 직접 입력과 기존 멀티뷰 작업 ID 입력을 구분한다. |
| 내보낼 형상 | ComfyUI와 REST의 `export_with_geometry` 기본값이 다르다. [원문 7](https://developers.tripo3d.ai/ko/docs/comfyui-nodes) · [원문 33](https://developers.tripo3d.ai/ko/docs/animations-retarget) | 메시 포함 애니메이션 파일이 필요하면 명시한다. |
| 면 수 범위 | 노드 UI가 허용하는 최대값과 P2의 삼각형·쿼드 상한이 다르다. [원문 7](https://developers.tripo3d.ai/ko/docs/comfyui-nodes) · [원문 16](https://developers.tripo3d.ai/ko/docs/generation-image-to-model/p) | 선택 모델과 토폴로지의 상한을 적용한다. |
| 변환 형식과 옵션 | 빠른 시작의 OPEN_EXR 선택지와 3MF 컬러 팁은 변환 표에서 확인되지 않거나 단색 안내와 충돌한다. [원문 2](https://developers.tripo3d.ai/ko/docs/quick-start) · [원문 30](https://developers.tripo3d.ai/ko/docs/models-convert) | 텍스처는 PNG로 먼저 시험한다. 3MF 색상은 출력 파일을 열어 확인한다. |
| 형식 변환의 영향 | 예제의 단순 변환 설명과 달리 변환 옵션으로 면 수, 피벗, 단위도 바꿀 수 있다. [원문 2](https://developers.tripo3d.ai/ko/docs/quick-start) · [원문 30](https://developers.tripo3d.ai/ko/docs/models-convert) | 기본 변환과 옵션을 추가한 변환의 결과·요금을 각각 계산한다. |
| v2와 폐기 기능 | 이전 가이드에는 v2 병행과 refine·stylize의 매핑이 남아 있다. SDK 문서는 v2 종료와 폐기를 안내한다. [원문 43](https://developers.tripo3d.ai/ko/docs/migration-v2-to-v3) · [원문 5](https://developers.tripo3d.ai/ko/docs/sdk) | 신규 작업은 v3의 현행 엔드포인트를 기준으로 한다. |
| 오류 번호 | 2018은 요청 실패와 비동기 작업 실패에서 가리키는 문제가 다르다. [원문 40](https://developers.tripo3d.ai/ko/docs/error-handling) · [원문 34](https://developers.tripo3d.ai/ko/docs/task-query) | HTTP 상태와 요청 code, 작업의 error_code를 함께 보존한다. |
| 빈도와 동시성 | 2000의 일반 설명만 보면 요청 빈도 제한과 진행 중 작업 수 제한을 혼동하기 쉽다. [원문 40](https://developers.tripo3d.ai/ko/docs/error-handling) · [원문 41](https://developers.tripo3d.ai/ko/docs/rate-limits) | 1007과 2000을 구분하고 헤더 및 카테고리별 활성 작업을 확인한다. |
| 파일 URL 수명 | 업로드 요청 예제와 결과 다운로드 URL은 발급 목적과 만료 기준이 다르다. [원문 37](https://developers.tripo3d.ai/ko/docs/files-presign) · [원문 2](https://developers.tripo3d.ai/ko/docs/quick-start) | 각 응답과 작업 재조회로 발급된 주소를 사용한다. |
| SmartSegment 입력 | 도입 설명은 토큰·URL 위주다. 11월 공지와 매개변수 표는 조건을 만족하는 task_id도 지원한다. [원문 26](https://developers.tripo3d.ai/ko/docs/mesh-smartsegment) · [원문 4](https://developers.tripo3d.ai/ko/docs/changelog) | 같은 키의 성공 작업 등 작업 ID 조건을 확인한다. |
| SDK 전용 항목 | Python SDK 설명에 image-to-depth가 등장하지만 한국어 목차에 독립 REST 명세가 없다. [원문 5](https://developers.tripo3d.ai/ko/docs/sdk) · [원문 1](https://developers.tripo3d.ai/ko/docs/introduction) | SDK 언급까지만 확인했다. 경로나 요청 필드는 추정하지 않는다. |

## 확인 범위

공개 한국어 문서의 설명·표·예제와 추가 가격표를 읽었다. 연결된 SDK 저장소의 전체 소스, 로그인 뒤 계정의 실제 한도, 예제 실행 결과는 별도 검증 범위다. 프로젝트에서 판단해야 할 항목은 모델 생성 성공 여부, 파일의 실제 재질·토폴로지, UE에서의 변형과 성능이다. [조회 목록 JSON](coverage.json) · [제작 검증안](ProductionWorkflow.md)
