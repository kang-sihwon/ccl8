# Tripo 메시·텍스처·파일 변환 분석

후처리는 성공한 원본 작업을 입력으로 받아 새 작업을 만든다. 각 단계의 작업 ID와 파일을 보존하면 형상이 만족스럽지 않을 때 앞 단계에서 다시 시작할 수 있다. 아래는 2026-11-18 문서의 계약과 제작 시 판단이다. [조사 지도](../Tripo-2026-11-18.md)

## 모델 가져오기와 정규화

`POST /v3/models/import`는 외부 모델의 file_token 또는 URL을 받는다. 공식 입력 목록은 GLB/GLTF/FBX/OBJ/STL, 최대 150 MB다. 업로드는 파일 참조를 만들고 가져오기는 처리 가능한 모델 작업을 만든다는 점을 구분한다. [가져오기](https://developers.tripo3d.ai/ko/docs/models-import)

성공 출력은 model_url, rendered_image_url과 정규화 값이다. 값이 제공되었을 때 원래 좌표를 복원하는 식은 다음과 같다.

```text
normalized = original * normalize_scale + normalize_translation
original = (normalized - normalize_translation) / normalize_scale
```

`normalize_scale=1`이어도 이동했을 수 있다. 파일/URL로 후처리를 요청하면 내부 가져오기에서 이 값들이 전달될 수 있지만, 일반적인 task_id 연결에서는 다시 가져오지 않으므로 없을 수 있다. 예전 작업에도 없으며 translation은 선택 필드다. SmartSegment는 항상 가져오는 예외로 공지되어 있다. [정규화 공지](https://developers.tripo3d.ai/ko/docs/changelog)

제안: 원래 크기·피벗이 중요한 건축 키트는 가져오기 전 치수와 이 두 값을 함께 저장한다. 이후 단계에 값이 없다는 이유로 단위 변환을 다시 적용하지 않는다.

## 텍스처 재생성

`POST /v3/models/texture`는 기존 모델에 텍스처를 다시 만든다. input은 task_id/file_token/URL이며 model, texture_seed, texture_quality, pbr, delight를 문서가 명시한다. 기본 model은 v3.5-20260815, quality는 standard, pbr은 true다. fast/standard/detailed/extreme를 지원하며 fast는 v2.5에서 거부된다. [텍스처](https://developers.tripo3d.ai/ko/docs/models-texture)

모델 생략 시 delight=false, v3.5 명시 시 기본 true라는 차이는 [생성 분석](Generation.md#텍스처-버전과-조명-제거)과 같다. 빠른 시작은 `texture_prompt`를, ComfyUI는 text_prompt/image_prompt/style_image를 안내하지만 REST 표에 해당 구조가 빠져 있다. 스타일을 특정해야 하는 실제 요청에서는 클라이언트의 전송 구조를 추가 확인해야 한다.

제안: 형상은 적합하고 표면만 어긋났다면 재생성보다 리텍스처를 먼저 비교한다. 털을 재질에 그려 넣는 결과와 실루엣에 털이 존재하는 결과는 따로 판정한다.

## 부품 분리의 세 경로

| 경로 | 입력과 버전 | 제어 | 용도 |
|---|---|---|---|
| 생성의 generate_parts | H3 이상 생성 요청 | 재질·쿼드·smart_low_poly 조합 제약 | 처음부터 분할된 형상 생성 |
| `POST /v3/mesh/segment` | 기존 모델 task_id/token/URL | 기본 v1.0-20250506, v2.0-20260430 베타 | 이미 채택한 모델 분리 |
| `POST /v3/mesh/smartsegment` | 이미지 또는 GLB | seg_type, granularity, hint, transform | 모델링과 분할을 묶거나 의미 라벨 분리 |

[Segment](https://developers.tripo3d.ai/ko/docs/mesh-segment)의 v1은 형상 기반이다. v2는 의미 라벨과 형상을 함께 사용한다. v2 전용 옵션은 `segmentation_granularity=simple/balanced/detailed`, `split_by_connectivity=true`, `ref_image`다. ref_image를 주면 앞의 두 옵션을 무시한다. v1에 ref_image를 보내면 오류다.

[SmartSegment](https://developers.tripo3d.ai/ko/docs/mesh-smartsegment)는 `seg_type=image`에서 PNG/JPEG/WebP를 모델링하고 분할한다. `seg_type=model`은 GLB만 받으며 column-major 4×4 변환 행렬 16개 값이 필수다. 단위 행렬은 다음과 같다.

```json
[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]
```

granularity는 coarse/medium/fine, 기본 medium이며 hint는 부품 식별 보조 텍스트다. 같은 API 키로 성공한 작업 ID도 입력으로 받는다. 이미지 경로는 해당 작업의 출력 이미지, 모델 경로는 GLB 출력을 사용한다. 출력에는 prompt, mask_url, seg_model_url, seg_task_id, model_task_id가 있다. 모델 다운로드 필드를 model_url 하나로 고정하면 분할 결과를 놓칠 수 있다.

제안: 캐릭터 몸과 무기·갑옷의 분리는 편집 출발점으로 사용한다. 분리 결과가 장착 피벗, 소켓, 교체 장비 경계를 충족하는지는 별도 검사한다.

## 분리한 부품의 보완

`POST /v3/mesh/complete`는 mesh/segment 작업 ID를 input으로 받는다. model은 v1.0-20250506, part_names는 결과 노드 이름 목록이며 생략하면 전체 부품을 처리한다. `tripo_part_0` 같은 실제 노드 이름을 사용한다. [메시 보완](https://developers.tripo3d.ai/ko/docs/mesh-complete)

| completion_mode | 동작 | 판단 |
|---|---|---|
| ai_completion, 기본 | AI로 보이지 않던 형상 보완 | 몸에서 떼어낸 부품의 숨은 면을 만들 후보 |
| quick_cap | 생성 없이 구멍 채우기 | 단순한 절단면을 막는 후보 |

두 모드 모두 새 작업을 만들며 결과를 다시 검토해야 한다. SmartSegment의 최상위 ID가 이 API의 입력 계약에 해당한다고 가정하지 않는다. 출력의 seg_task_id를 쓰려면 해당 하위 작업이 성공한 segment 결과인지 확인한다.

## 면 수 감소와 UV

| 기능 | 모델·입력 | 한도와 옵션 | 출력 판단 |
|---|---|---|---|
| `POST /v3/mesh/decimate` | 기본 v2.0. 표에는 task_id/file_token | v2/v3 삼각형 500에서 20,000, 쿼드 500에서 10,000 | 별도 LOD 후보 생성 |
| decimate v1.0 | face_limit 필수 | 삼각형 최대 2,000,000, 쿼드 최대 150,000 | 기본 감소용 |
| decimate v3.0 | 명시 필요 | quad=false, bake=true, part_names 선택, smart_uv=false | smart_uv=true는 추가 비용 없이 UV도 생성 |
| `POST /v3/mesh/uv` | task_id/token/URL, model=default | 등가 삼각형 80,000 이하, bake=true | model_url와 uv_render_url |

근거: [리토폴로지](https://developers.tripo3d.ai/ko/docs/mesh-decimate), [Smart UV](https://developers.tripo3d.ai/ko/docs/mesh-uv).

smart_uv는 decimate v3에서만 지원하며 v1/v2에 보내면 400/1004다. 이 옵션은 UV 미리보기를 제공하지 않는다. 별도 Smart UV는 기존 텍스처·리깅·애니메이션을 새 레이아웃에 재매핑하고 미리보기도 제공한다. bake=false이면 UV가 있는 형상만 반환한다. export_orientation과 compress=geometry도 선택할 수 있다.

80,000 등가 삼각형은 쿼드 한 개를 삼각형 두 개로 계산한다. 초과 입력은 접수 후 작업 실패가 될 수 있으므로 작업 생성 성공만 확인해서는 안 된다. 먼저 면 수를 줄이는 순서를 제안한다.

## 형식 변환의 전체 옵션군

`POST /v3/models/convert`의 필수 값은 input과 format이다. input 표는 task_id/file_token을 명시한다. format은 GLTF/USDZ/FBX/OBJ/STL/3MF이며 3MF는 단색으로 안내한다. GLB라는 별도 format 값을 임의로 넣지 않는다. [변환](https://developers.tripo3d.ai/ko/docs/models-convert)

| 옵션군 | 필드와 기본값 | 조건 |
|---|---|---|
| 메시 | quad=false, force_symmetry=false, face_limit 생략 | force_symmetry는 quad일 때만. 면 수 생략 시 유지 |
| 바닥 | flatten_bottom=false, flatten_bottom_threshold=0.01 | 프린팅용 후보. 캐릭터 발 형태 변경 여부 확인 |
| 텍스처 | texture_size=4096, texture_format=JPEG, bake=true, pack_uv=false | 지원 형식 BMP/DPX/HDR/JPEG/PNG/TARGA/TIFF/WEBP |
| 버텍스 | export_vertex_colors=false | OBJ/GLTF에서만 |
| 좌표 | pivot_to_center_bottom=false, scale_factor=1, export_orientation=+x | 방향 +x/-x/+y/-y |
| 애니메이션 | with_animation=true, animate_in_place=false | 뼈대·애니메이션 유지 여부 및 이동 처리 |
| 부품 | part_names | 분할 결과에서 선택한 부품만 출력 |
| FBX 호환 | fbx_preset=blender | blender/3dsmax/mixamo/bake_scale |
| FBX 텍스처 | texture_packaging=embedded | zip이면 .fbx와 .fbm 텍스처 폴더를 함께 반환 |

texture_packaging=zip은 model_url이 ZIP을 가리키며 FBX 외 형식에서 보내면 400 오류다. 파일 확장자를 작업 유형만 보고 결정하지 않는다. 압축 해제 후 FBX와 .fbm 폴더를 함께 둔다.

제안: UE 반입용 파일과 원본 파일을 각각 보존하고 최종 변환에서 크기·방향을 검증한다. `spec=unreal`은 리깅 옵션이고 `fbx_preset` 목록에는 unreal이 없으므로 서로 섞지 않는다. 변환이 면 수·피벗을 바꿀 수 있으므로 단순 확장자 변경으로만 취급하지 않는다.
