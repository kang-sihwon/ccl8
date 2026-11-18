# Tripo 이미지·3D 생성 분석

콘셉트의 외형을 유지하려면 이미지 입력을 먼저 검토한다. 텍스트 입력은 탐색 속도가 빠르지만, 이미 확정한 얼굴·의상·장식의 배치를 전달할 정보가 줄어든다. 이는 제작 판단이며, 아래 표는 2026-11-18 공식 문서의 API 계약이다. [조사 지도](../Tripo-2026-11-18.md)

## 지오메트리 모델 선택

| 모델 | 문서상 용도 | 면 수·지원 범위 | 선택 비용 |
|---|---|---|---|
| `v3.1-20260211` | H3.1 고정밀 형상 | `geometry_quality`, `smart_low_poly`, `generate_parts`, `quad` | 디테일 후보. 후처리와 실제 면 수 검사 필요 |
| `v3.0-20250812` | H3.0 이전 세대 | 고급 옵션 지원 | 기존 결과와의 호환 비교용 |
| `v2.5-20250123` | H2.5 레거시 | H3 전용 옵션을 같은 방식으로 기대할 수 없음 | 새 작업의 기본 후보로 삼지 않음 |
| `P1-20260311` | 로우폴리 | 50에서 20,000면, 쿼드 불가 | P2보다 저렴한 후보 |
| `P2-20260801` | 면 수 제어·네이티브 쿼드 | 삼각형 48에서 50,000면, 쿼드 48에서 25,000면 | 높은 기본 생성비와 편집 가능한 토폴로지를 비교 |

P 시리즈에는 `smart_low_poly`, `generate_parts`, `geometry_quality`를 보내지 않는다. P2의 `quad=true`는 FBX 출력을 강제하며, 면 수를 생략하면 10,000쿼드로 안내한다. 일반 적응형 기본값과 구분해야 한다. H의 `face_limit`는 최대 면 수로 설명되지만 동일한 숫자 범위가 명시되어 있지 않다. [모델 목록](https://developers.tripo3d.ai/ko/docs/models-and-versions), [P 텍스트](https://developers.tripo3d.ai/ko/docs/generation-text-to-model/p)

P 시리즈의 약 2초 메시 생성, H의 최고 품질이라는 표현은 업체 설명이다. 대기열·텍스처·다운로드를 포함한 총시간이나 ccl8에 맞는 변형 품질을 측정한 결과가 아니다.

## 생성 엔드포인트와 입력

모든 경로는 `POST /v3/generation/` 아래에 있고 비동기 `task_id`를 반환한다. 다음 세 기능은 H/P가 같은 엔드포인트를 사용하며 `model`로 구분한다.

| 기능 | 필수 입력 | 이 기능에서 확인할 옵션 | 원문 |
|---|---|---|---|
| `text-to-model` | `prompt`, `model` | 프롬프트 최대 1,024자, `negative_prompt` 255자, `image_seed`, `model_seed` | [H](https://developers.tripo3d.ai/ko/docs/generation-text-to-model/standard), [P](https://developers.tripo3d.ai/ko/docs/generation-text-to-model/p) |
| `image-to-model` | `input`, `model` | `enable_image_autofix=false`, `texture_alignment=original_image`, `orientation=default` | [H](https://developers.tripo3d.ai/ko/docs/generation-image-to-model/standard), [P](https://developers.tripo3d.ai/ko/docs/generation-image-to-model/p) |
| `multiview-to-model` | `inputs`, `model` | 입력 구조 세 가지 중 하나. P 페이지는 정렬·방향·모델 시드도 기재 | [H](https://developers.tripo3d.ai/ko/docs/generation-multiview-to-model/standard), [P](https://developers.tripo3d.ai/ko/docs/generation-multiview-to-model/p) |

단일 이미지의 `input`에는 업로드 토큰, 접근 가능한 URL 또는 적합한 선행 작업 ID를 넣는다. `C:\\...` 로컬 경로는 REST 서버에서 읽을 수 없으므로 업로드해야 한다. CLI의 로컬 파일 입력은 이 처리를 감싼 사용 방법이다.

`texture_alignment=original_image`는 참조 이미지 일치, `geometry`는 형상 일치를 우선한다. `orientation=align_image`는 입력 시점에 맞춘 정렬이다. 자동 이미지 보정은 외형 충실도에 영향을 줄 수 있으므로, 확정 콘셉트 비교에서는 기본 false를 유지하는 방안을 제안한다.

## 공통 옵션과 충돌

| 옵션군 | 문서의 값·기본값 | 적용 조건 |
|---|---|---|
| 재질 | `texture=true`, `pbr=true` | PBR을 켜면 texture도 켜진다. 형상만 원하면 둘 다 false |
| 텍스처 | `texture_seed` 무작위, `texture_quality=standard` | `fast/standard/detailed/extreme`. fast는 v3.5 전용 |
| 형상 | `face_limit` 적응형, `geometry_quality=standard` | detailed는 H 고급 형상용 |
| 실물 크기 | `auto_size=false` | true는 미터 기준 크기 추정. 실제 크기 검증을 대체하지 않음 |
| 편집 | `quad=false`, `smart_low_poly=false`, `generate_parts=false` | H3 이상 고급 옵션. P는 P2 quad만 지원 |
| 압축 | `compress=geometry` | H3 이상 meshopt 출력 옵션 |
| UV | `export_uv=true` | P 페이지에 기재. false면 UV 생략 |
| 방향 | `export_orientation` 기본 +x | 허용 방향 +x/-x/+y/-y. 후처리 예정이면 생성 단계에서는 생략 |

H의 `generate_parts`는 조합 제약이 크다. `texture`나 `pbr`가 true이면 1004 오류, `quad=true`이면 쿼드 무시, `smart_low_poly=true`이면 부품 생성이 무시된다. 부품 생성 목적이라면 두 재질 옵션을 false로 명시하고 quad와 smart_low_poly를 생략한다. [H 생성 제약](https://developers.tripo3d.ai/ko/docs/generation-text-to-model/standard)

생성 단계의 `export_orientation`은 이후 텍스처·리깅·변환의 방향을 틀리게 만들 수 있다. 문서는 이 경우에도 `status=success`가 나올 수 있다고 경고한다. 마지막 형식 변환에서 방향을 지정하는 흐름을 따른다. [이미지 생성의 방향 주의사항](https://developers.tripo3d.ai/ko/docs/generation-image-to-model/standard)

## 텍스처 버전과 조명 제거

지오메트리 모델과 텍스처 모델은 별도로 고정한다. 2026-11 공지에 따라 v2.5 지오메트리를 제외한 생성에서 텍스처 버전을 생략하면 v3.5와 `delight=false`가 적용된다. v3.5를 명시하면 delight의 기본값은 true다. 같은 버전처럼 보여도 생략 여부에 따라 참조 조명 처리 방식이 달라진다. [변경 공지](https://developers.tripo3d.ai/ko/docs/changelog)

| 요청 | 문서상 텍스처 처리 | 대응 |
|---|---|---|
| v2.5 형상, texture_version 생략 | 텍스처 v2.5 | fast 사용 금지 |
| 그 외 형상, texture_version 생략 | v3.5, delight=false | 비교 시험에서는 버전·delight 둘 다 기록 |
| texture_version=v3.5 명시 | v3.5, delight 기본 true | 조명 제거가 목적이면 true 명시 |
| texture_version=v3.0, delight 생략 | v3.5, delight=false | 이전 v3.0 재현으로 간주하지 않음 |
| texture_version=v3.0, delight 명시 | 설명에 해석 여지 있음 | [불일치 목록](Sources.md#문서-불일치와-대응) 참조 |

fast는 standard와 해상도·요금이 같고 세부 표현을 줄여 속도를 높이는 선택지다. 상세한 표면이 필요한 보스 최종 후보에 쓰기 전 비교가 필요하다.

## 멀티뷰의 세 입력 형식

권장하는 뷰 키 형식은 정면을 포함한 최소 두 시점이 필요하다. 각 객체에 front/left/back/right 중 하나만 넣으며 순서는 무관하다. 아래 토큰은 설명용 자리표시자다.

```json
{
  "model": "P2-20260801",
  "inputs": [
    {"front": "file_front_token"},
    {"back": "file_back_token"}
  ],
  "face_limit": 12000,
  "texture": true,
  "pbr": true,
  "texture_version": "v3.5-20260815",
  "delight": true
}
```

레거시 배열은 `[front, left, back, right]`의 네 슬롯을 고정하고 빠진 시점을 빈 문자열로 둔다. 선행 멀티뷰 결과를 재사용할 때는 `[{"task_id":"<successful_multiview_task_id>"}]` 하나만 보낸다. 원본은 성공한 `image_to_multiview` 또는 `edit_multiview`여야 한다. 세 형식을 섞을 수 없다. [멀티뷰 입력 계약](https://developers.tripo3d.ai/ko/docs/generation-multiview-to-model/standard)

하나의 턴어라운드 보드에 여러 그림을 합친 이미지는 이 배열과 다르다. 각각 분리한 뷰를 전달하고, 뿔 개수·장식 위치·팔다리 비율이 모든 시점에서 같은지 검토하는 방안을 제안한다. 자동 생성한 뒤쪽 모습은 원래 디자인을 관측한 정보가 아니라 생성된 해석이다.

## 이미지 생성·편집 네 기능

| 경로 | 필수 입력 | 결과와 제약 |
|---|---|---|
| `text-to-image` | `prompt` 최대 1,024자 | 기본 seedream_v4. 이미지 작업 ID 반환 |
| `image-to-image` | `input`, template 없으면 `prompt` | 기본 seedream_v5. `inputs`의 seedream은 최대 4장, banana는 최대 10장 |
| `image-to-multiview` | URL 또는 file_token인 `input` | 정면·왼쪽·뒤·오른쪽 이미지 생성 |
| `edit-multiview` | 성공한 image_to_multiview ID인 `input`, `prompts` | 서로 다른 view 1개에서 4개, 각 prompt는 비어 있지 않고 최대 1,024자 |

원문: [텍스트 이미지](https://developers.tripo3d.ai/ko/docs/generation-text-to-image), [이미지 편집](https://developers.tripo3d.ai/ko/docs/generation-image-to-image), [멀티뷰 생성](https://developers.tripo3d.ai/ko/docs/generation-image-to-multiview), [멀티뷰 편집](https://developers.tripo3d.ai/ko/docs/generation-edit-multiview).

멀티뷰 출력은 `output.generate_multiview_image` 안의 `front_view_url`, `left_view_url`, `back_view_url`, `right_view_url`로 안내한다. 편집 입력은 URL이나 파일 토큰을 받지 않는다. 편집 결과를 다시 편집하는 재귀 연결은 이 페이지에서 보장하지 않으므로 임의로 확장하지 않는다.

| 이미지 옵션 | 지원 범위 | 사용 판단 |
|---|---|---|
| model | seedream_v4/v5, banana/banana_pro/banana2, chat_image 계열 | seedream_v4는 이미지 편집 미지원 |
| quality | chat_image_2: low/medium/high. 2.5 flare/sunburst: xhigh/max 추가 | 기본 low, auto 불가 |
| background | 2.5만 auto/opaque/transparent | 타 모델은 무시. transparent는 output_format=png 필요 |
| template | asset_extraction, character_completion, t_pose, head_extraction, 3d_enhance, variants, print_clay, figure | 모델 입력 정리에 활용할 후보 |
| t_pose, sketch_to_render | 기본 false | 포즈 정리·스케치 렌더링 |
| inputs, prompt 참조 | seedream/banana 장수 제한. prompt에서 이미지 번호 참조 | 단일 input과 inputs의 우선순위는 불명확 |

chat_image_1은 2026-10-23 종료로, 1.5는 2026-12-01 종료 예정으로 안내한다. flare는 속도, sunburst는 편집하지 않은 부분 보존에 초점을 둔다는 업체 설명이다. 이미지 생성 문서의 예제에는 지원 표와 다른 `seedream-v4` 표기가 남아 있어 그대로 복사하지 않는다. [모델 현황](https://developers.tripo3d.ai/ko/docs/models-and-versions)

사용자 지정 크기는 변경 공지에만 상세히 있다. chat_image_2/2.5의 각 변은 16의 배수이면서 최대 3,840, 긴 변은 짧은 변의 3배 이하, 총 픽셀은 655,360에서 8,294,400이다. `size`와 `output_format`은 엔드포인트 표가 불완전하므로 실제 사용 전 SDK/CLI 요청 명세를 확인한다. [이미지 변경 공지](https://developers.tripo3d.ai/ko/docs/changelog)

## Gaussian Splat

`POST /v3/generation/image-to-splat`은 PNG/JPEG/WebP 한 장을 받아 .splat을 출력한다. input은 토큰 또는 공개 URL이고 model_seed를 선택할 수 있다. 최대 20 MB, 권장 최소 256×256, 비용 30크레딧, 약 4분이라는 문서 안내다. [Splat 원문](https://developers.tripo3d.ai/ko/docs/generation-image-to-splat)

이 출력은 편집 가능한 메시·PBR·리깅 파이프라인과 용도가 다르다. ccl8의 움직이는 몬스터나 충돌 소품 제작에는 image-to-model을 먼저 시험하는 방안을 제안한다.
