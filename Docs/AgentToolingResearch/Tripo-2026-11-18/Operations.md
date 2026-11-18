# Tripo 운영·요금·오류·마이그레이션 분석

Tripo 연동은 작업 생성과 결과 수집을 분리해 관리해야 한다. 요청 응답을 잃거나 로컬 대기가 끝나도 서버의 작업은 남을 수 있다. 작업 ID를 보존하고 조회를 재개해야 중복 생성 비용을 피할 수 있다. [오류 처리](https://developers.tripo3d.ai/ko/docs/error-handling), [조사 지도](../Tripo-2026-11-18.md)

## 인증과 공통 응답

글로벌 base URL은 `https://openapi.tripo3d.ai/v3`, 중국은 .com 도메인이다. 요청에는 `Authorization: Bearer <api_key>`를 사용하고 JSON 요청의 Content-Type은 application/json이다. 키는 서버 또는 로컬 제작 프로세스에서 보관한다. 공개 게임 클라이언트에 넣는 구조는 피한다. [소개](https://developers.tripo3d.ai/ko/docs/introduction), [인증](https://developers.tripo3d.ai/ko/docs/authentication)

생성 접수는 보통 `code=0`과 data.task_id를 반환한다. 이것은 모델 완성이 아니다. 요청 오류는 code/status/message/suggestion/request_id를 사용할 수 있으며, request_id 등은 없을 수 있다. 일부 SDK가 error_msg를 제공해도 REST 실패 상세의 이름은 error_message로 확인한다.

서명된 파일 다운로드나 스토리지 PUT에는 Tripo API 키 헤더를 보내지 않는다. API 인증과 파일 URL의 임시 권한을 분리한다. [SDK 다운로드](https://developers.tripo3d.ai/ko/docs/sdk), [presign](https://developers.tripo3d.ai/ko/docs/files-presign)

## 작업 수명과 조회

| 상태 | 처리 |
|---|---|
| queued / running | 아직 미완료. progress는 0에서 100 |
| success | 작업별 output을 해석하고 파일 다운로드 |
| failed / cancelled | 폴링 종료, 실패 상세 표시 |
| banned / expired | 현재는 failed로 외부 표시. 레거시 응답에서도 종료 처리 |
| unknown 또는 미지의 값 | 성공·실패로 단정하지 않고 제한 시간 동안 조회 |

[수명주기](https://developers.tripo3d.ai/ko/docs/task-lifecycle)는 1초에서 2초 폴링을 권장한다. 문서의 5분 대기 제한과 SDK 예제의 20분은 클라이언트 예시이며 서비스 완료 시간 보장이 아니다. 제한 시간이 끝나면 task_id와 현재 상태를 보관한다.

| API | 요청 | 응답·주의 |
|---|---|---|
| GET /v3/tasks/{task_id} | 생성 응답의 실제 ID | type/status/progress/output/credits_consumed/created_at/completed_at 및 실패 상세 |
| POST /v3/tasks/list | task_ids 배열, 최대 100개 | tasks는 ID별 객체. missed는 없음·접근 불가 ID 목록 |
| 일괄 쿼리의 잘못된 UUID | 하나라도 형식 오류 | 전체 요청이 400/1004 |

근거: [단일 조회](https://developers.tripo3d.ai/ko/docs/task-query), [일괄 조회](https://developers.tripo3d.ai/ko/docs/task-batch-query). 모든 ID가 있으면 missed는 null 또는 생략될 수 있다. 누락을 곧바로 삭제로 해석하지 않고 키·계정을 확인한다.

모델은 보통 output.model_url, 미리보기는 rendered_image_url이다. SmartSegment의 seg_model_url과 멀티뷰의 네 이미지 URL처럼 작업별 출력 구조가 다르다. 빠른 시작은 결과 URL이 약 5분 후 만료된다고 안내하고 SDK는 재조회 후 다운로드를 권한다. 장기 보관할 대상은 임시 URL이 아니라 실제 파일과 작업 기록이다. [빠른 시작](https://developers.tripo3d.ai/ko/docs/quick-start)

## 파일 업로드

| 경로 | 요청·결과 | 적용 |
|---|---|---|
| POST /v3/files | multipart file → file_token | JPEG/PNG 최대 20 MB, 모델 GLB/GLTF/FBX/OBJ/STL 최대 150 MB |
| POST /v3/files/presign | 점 없는 format → presigned_url/file_token/expires_in | 반환 URL로 바이트 PUT 후 토큰 사용 |
| POST /v3/files/upload-credentials | file_token/upload_url/expires_at 예제 | 파일 문서에 있으나 SDK 공개 흐름은 미제공으로 설명 |

[파일 관리](https://developers.tripo3d.ai/ko/docs/files), [대용량 업로드](https://developers.tripo3d.ai/ko/docs/files-presign). presign의 expires_in은 실제 URL 서명 만료나 토큰 유효 기간을 보장하지 않는다고 명시한다. 즉시 PUT하고, 만료되면 URL과 토큰을 새로 받아 다시 업로드한다.

WebP는 생성·presign에서 안내하지만 multipart 파일 표에서는 빠져 있다. 업로드와 후속 모델 처리의 허용 확장자가 같다고 가정하지 않는다. 또한 모델 import와 rig-check의 형식 목록도 다르다. 파일명만 바꾸는 변환은 사용하지 않는다.

## 크레딧과 요금

작업을 만들면 필요한 크레딧을 동결하고, 성공 시 소비한다. 실패·취소 시 동결분이 해제된다. `balance`는 사용 가능한 잔액이고 frozen은 진행 작업의 예약분이다. 두 값과 credits_consumed는 소수점이 있는 JSON number이므로 정수로 잘라 읽지 않는다. [청구](https://developers.tripo3d.ai/ko/docs/billing), [계정](https://developers.tripo3d.ai/ko/docs/account)

공식 공지는 2026-12-01부터 콘솔 표시를 USD로 바꾸고 API 단위는 크레딧을 유지한다고 설명한다. 환산은 1크레딧=$0.01이며 이 금액은 서비스 잔액 가치다. 실제 과거 결제액이나 인출 가능 금액과 같다는 뜻은 아니다.

아래는 2026-11-18 [공식 가격표](https://developers.tripo3d.ai/ko/pricing)의 공개 단가다. 할인·계정별 조건·향후 변경을 적용한 청구 검증값은 아니다.

| 3D 생성 | 형상만 | 표준 텍스처 포함 |
|---|---:|---:|
| H 텍스트 | 10 | 20 |
| H 이미지 / 멀티뷰 | 20 | 30 |
| P1 텍스트 | 30 | 40 |
| P1 이미지 / 멀티뷰 | 40 | 50 |
| P2 텍스트 / 이미지 / 멀티뷰 | 100 | 110 |
| image-to-splat | 30 | 해당 없음 |

단위는 크레딧이다. H의 옵션 가산은 detailed 텍스처 +10, extreme +20, detailed 형상 +20, quad +5, smart_low_poly +10, generate_parts +20으로 표시된다. 지원되지 않는 조합을 단순 합산하지 않는다. P2 텍스처 포함 standard/detailed/extreme는 110/120/130으로 공지한다. P2 quad의 별도 가산은 이 표에서 명확하지 않으므로 실제 계획·청구를 확인한다. [P2 비용 공지](https://developers.tripo3d.ai/ko/docs/changelog)

| 처리 | 크레딧 | 과금 단위 |
|---|---:|---|
| 리텍스처 fast / standard / detailed / extreme | 10 / 10 / 20 / 30 | 작업 |
| 변환 기본 / 고급 | 5 / 10 | 작업 |
| 리토폴로지 v1 / v2 | 10 / 30 | 작업 |
| 리토폴로지 v3 삼각형 / 쿼드 | 35 / 40 | 작업 |
| Smart UV | 20 | 작업 |
| Segment | 40 | 작업 |
| Complete AI / quick_cap | 50 / 30 | 작업 |
| SmartSegment 이미지 / 모델 | 85 / 55 | 작업 |
| rig-check / rig | 0 / 25 | 작업 |
| retarget | 10 | 애니메이션 하나 |
| image-to-multiview | 10 | 작업 |
| edit-multiview | 5 | 편집 이미지 하나 |

고급 변환은 quad/face_limit/flatten_bottom/flatten_bottom_threshold/texture_size/texture_format/pivot_to_center_bottom/scale_factor 중 기본값이 아닌 값을 사용하면 10크레딧으로 안내한다. 리토폴로지 v3의 smart_uv는 추가 요금이 없다. 별도 Smart UV의 미리보기나 보존 기능이 필요하지 않으면 두 작업을 무조건 연속 실행할 이유가 없다.

| 이미지 모델·품질 | 1K 이하 | 2K | 4K |
|---|---:|---:|---:|
| seedream_v4/v5 | 5 | 5 | 5 |
| banana | 5 | 미기재 | 미기재 |
| banana2 | 10 | 10 | 15 |
| banana_pro | 15 | 15 | 20 |
| chat_image_2 / 2.5 기본 | 10 | 10 | 15 |
| chat_image_2 / 2.5 high | 20 | 25 | 40 |
| chat_image_2.5 xhigh | 25 | 30 | 45 |
| chat_image_2.5 max | 30 | 50 | 50 |
| 종료된 chat_image_1 | 5 | 미기재 | 미기재 |
| 종료 예정 chat_image_1.5 | 10 | 미기재 | 미기재 |

quality 생략·low·medium은 기본 요금, size=auto는 2K 구간이라고 가격표가 안내한다. 종료 모델의 가격 행 존재를 현재 사용 가능 근거로 삼지 않는다. 플랫폼 최저 충전 표시는 $1이며 결제·크레딧 충전은 이 조사에서 수행하지 않았다.

## 계정 조회와 자동 정산

`GET /v3/account/balance`는 balance/frozen, `GET /v3/account/usage`는 작업 ID·유형·크레딧·생성 시각을 반환한다. 새 `GET /v3/account/usage/history`는 기간별 자동 정산용으로 구분한다. [계정](https://developers.tripo3d.ai/ko/docs/account), [사용 내역](https://developers.tripo3d.ai/ko/docs/account-usage-history)

| usage/history 조건 | 규칙 |
|---|---|
| 범위 | 호출한 API 키가 생성한 성공 작업만 |
| 시각 | created_at 기준, start_time 포함 / end_time 미포함 |
| 기간 | 시간대가 있는 RFC 3339, 최대 31일 |
| 페이지 | page_size 기본 100, 1에서 100. next_cursor 사용 |
| 페이지 이동 | 같은 start_time/end_time 유지 |
| 출력 | items, has_more, next_cursor. 프롬프트·입력·다운로드 링크 없음 |
| 수집 시점 | 종료 경계가 현재보다 30분 이상 지난 기간 권장 |
| 제한 | API 키당 분당 60회. 429/1007 또는 503/1008에 Retry-After |

제안: 비용 합산에는 소수형을 사용하고 작업별 실제 credits_consumed를 저장한다. 실패·실행 중 작업은 history에 없으므로 이 API만으로 전체 작업 이력이나 실패율을 만들지 않는다. 여러 API 키를 사용하면 키마다 조회한다.

## 오류와 재시도

| HTTP·코드 | 분류 | 대응 |
|---|---|---|
| 401: 2 / 1002, 개별 문서의 4000 | 인증 | 키·리전·권한 확인 |
| 400: 1003 / 1004 | 본문·파라미터 | JSON, 타입, 옵션 조합 수정 |
| 404: 2001 / 4001 | 작업·엔드포인트 | ID, API 키, 경로 확인 |
| 400: 2002 | 미지원 작업 | 엔드포인트·버전 확인 |
| 400: 2003 / 2004 | 빈 파일·미지원 형식 | 파일과 업로드 확인 |
| 400: 2006 / 2007 | 선행 작업 타입·상태 | 맞는 결과가 success인지 확인 |
| 400: 2008 | 콘텐츠 거절 | 입력 검토. 같은 요청 무한 반복 금지 |
| 403: 2010 | 잔액 부족 | 진행 작업의 frozen과 가용 잔액 확인 |
| 400: 2015 / 2018 | 버전 종료·복잡한 리메시 | 현행 버전·입력 복잡성 검토 |
| 429: 1007 / 2000 | 요청 빈도·작업 동시성 | 서버 헤더에 따른 대기 |
| 500: 1000 / 1001 | 서버 오류 | 접수 여부 확인 후 안전한 경우 재시도 |

[통합 오류](https://developers.tripo3d.ai/ko/docs/error-handling)와 개별 엔드포인트를 합쳐 분류했다. 조회가 성공한 실패 작업의 `error_code`는 요청 오류 code와 구분한다. 예를 들어 2018은 요청에서는 리메시 복잡성, 작업 실패에서는 처리 시간 초과 등으로 안내된다. 실패 작업의 2020/2021/2022는 입력 다운로드 실패/파일 크기 초과/생성 이미지 크기 초과다. [실패 상세](https://developers.tripo3d.ai/ko/docs/task-query)

제안: 재시도 정책을 GET 조회와 생성 POST로 나눈다. 429는 Retry-After와 X-RateLimit-Reset을 따르고 지수 백오프를 적용한다. 생성의 5xx·네트워크 단절은 제출 여부가 불명확하므로 바로 다시 보내지 않는다. 문서에는 일반적인 멱등성 키 계약이 없어 클라이언트의 요청 기록만으로 서버 중복 생성을 완전히 막는다고 보장할 수 없다.

## 속도 제한과 동시성

속도 제한은 API 키별 요청 빈도, 동시성은 계정별 카테고리의 진행 작업 수다. 키를 여러 개 만드는 것으로 계정 동시성이 늘어난다고 가정하지 않는다. [제한 문서](https://developers.tripo3d.ai/ko/docs/rate-limits)

| 카테고리 | 공개 기본 동시성 |
|---|---:|
| H 3D 생성 | 10 |
| P 3D 생성 | 5 |
| 이미지 생성·편집·멀티뷰 | 1 |
| 애니메이션·리그 검사 | 10 |
| 모델 처리 | 5 |
| 메시 작업 | 10 |

카테고리끼리는 독립 풀이다. 응답 헤더 X-RateLimit-Limit/Remaining/Reset을 확인한다. 모든 기능의 고정 RPM은 공개 표에 없으며 계정별 변경도 가능하다. Smart UV·SmartSegment의 카테고리 배정은 해당 표에 명시되지 않으므로 임의로 확정하지 않는다.

## Webhook

[Webhook 문서](https://developers.tripo3d.ai/ko/docs/webhooks)는 HTTPS 수신 주소와 이벤트 구독을 설정하는 흐름을 제공한다. 이벤트는 task.completed, task.failed, balance.low이며 task.failed에는 취소도 포함된다.

Tripo-Webhook-Signature의 t/v1을 읽어 `timestamp + "." + raw_body`의 HMAC-SHA256을 계산한다. 16진수 서명을 상수 시간으로 비교하고, Tripo-Webhook-Delivery로 중복 전달을 제거한다. Tripo-Webhook-Id는 엔드포인트 ID이므로 전달 ID와 구분한다. 약 5분보다 오래된 타임스탬프를 거부하는 방안도 문서에 있다.

제안: 서명 검증 뒤 빠르게 2xx를 반환하고 무거운 처리는 큐로 넘긴다. 전체 페이로드 스키마, 재전송 간격·최대 횟수·보관 기간은 공개 페이지에 없어 실제 연동 전에 확인한다. 파일 다운로드 복구를 위한 작업 조회도 유지한다.

## v2에서 v3로 전환

[마이그레이션](https://developers.tripo3d.ai/ko/docs/migration-v2-to-v3)의 구조 변경은 base URL 교체, 범용 task+type을 기능별 경로로 분리, 입력을 input으로 통합, create_time/consumed_credit를 created_at/credits_consumed로 교체하는 것이다. text/image 생성도 각각의 엔드포인트로 나뉜다.

| v2 유형·경로 | v3 대응 |
|---|---|
| text_to_model / image_to_model / multiview_to_model | generation 아래 같은 이름의 하이픈 경로 |
| text_to_image / generate_image | generation/text-to-image / image-to-image |
| generate_multiview_image / edit_multiview_image | generation/image-to-multiview / edit-multiview |
| convert_model / import_model / texture_model | models/convert / import / texture |
| animate_prerigcheck / animate_rig / animate_retarget | animations/rig-check / rig / retarget |
| mesh_segmentation / mesh_completion / highpoly_to_lowpoly | mesh/segment / complete / decimate |
| GET v2 task / POST v2 upload | GET tasks/{id} / POST files |
| refine_model / stylize_model | 이전 매핑에 남지만 SDK·ComfyUI에서 폐기 명시 |

v2/v3 병행 가능이라는 이전 안내와 2026-10-31 v2 종료 안내가 함께 남아 있다. 새 연동은 v3로 작성하고 구형 refine/stylize 경로를 복원하지 않는다. API 키 재사용과 기존 작업 조회 호환성은 별개다. SDK를 올린 뒤 입력·상태·파일·실제 비용을 기능별로 검증하는 순서를 제안한다.

마이그레이션 페이지의 다운로드 Skill과 설치 명령은 참고 자료다. 이 연구에서 해당 외부 Skill을 설치하거나 지시를 실행하지 않았다.
