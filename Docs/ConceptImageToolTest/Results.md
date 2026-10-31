# 시험 결과

컨셉 이미지 도구 비교의 결과 지도다. 상위 문서는 [컨셉 이미지 도구 비교 테스트](../ConceptImageToolTest.md)다. 도구는 아직 정하지 않았다.

| 문서 | 읽는 때 |
|---|---|
| [환경](Results/Scenes.md) | T1, T2의 장별 점수와 관찰이 필요할 때 |
| [캐릭터와 몹](Results/Characters.md) | T3-T6의 장별 점수와 관찰이 필요할 때 |
| [소품](Results/Props.md) | T7의 점수와 이미지→3D 입력 후보가 필요할 때 |
| [화풍 잇기](Results/StyleChain.md) | T0, T8의 참조와 내용 번짐이 필요할 때 |
| [게임 시안](Results/GameConcept.md) | T9-T12의 수정, 3면도, 단독 에셋 결과가 필요할 때 |

## 요약

| 시험 | 대상 | 점수가 가장 높은 도구 | 사용자 순위 |
|---|---|---|---|
| T1 영어 | 환경 | Nano Banana Pro | Midjourney, FLUX 2, Gemini 웹, Nano Banana Pro, GPT Image 2.5 |
| T1 한국어 | 환경 | 서술 이해는 Nano Banana Pro, GPT Image 2.5, FLUX 2 | Midjourney, FLUX 2, Nano Banana Pro, GPT Image 2.5 |
| T2 | 보스 공간 | 스타일 적합은 Midjourney, 지시 반영과 모델링 활용도는 FLUX 2 | Midjourney, FLUX 2, GPT Image 2.5, Nano Banana Pro |
| T3 | 캐릭터 | GPT Image 2.5 | GPT Image 2.5, Midjourney, FLUX 2, Nano Banana Pro |
| T4, T5 | 부분 수정, 3면도 | GPT Image 2.5 | 매기지 않음 |
| T6 | 몹 실루엣 | GPT Image 2.5 | GPT Image 2.5, Midjourney, FLUX 2, Nano Banana Pro |
| T7 | 소품 | GPT Image 2.5 | GPT Image 2.5, Nano Banana Pro, FLUX 2, Midjourney |
| T8a | 화풍 잇기 | 톤 유지는 GPT Image 2.5, 번짐 없음은 FLUX 2 | Midjourney, FLUX 2, Nano Banana Pro, GPT Image 2.5(실패) |
| T8b | 화풍 잇기 | 톤 유지는 GPT Image 2.5, 번짐 없음은 Nano Banana Pro, FLUX 2, Midjourney | Midjourney, Nano Banana Pro, GPT Image 2.5, FLUX 2 |
| T8c | 화풍 잇기 | 톤 유지는 Midjourney | GPT Image 2.5, Nano Banana Pro, FLUX 2, Midjourney(실패) |
| T9-T11 | 시안 수정, 3면도, 보스 공간 | GPT Image 2.5 | GPT Image 2.5, Nano Banana Pro, FLUX 2 |
| T12 | 단독 에셋 | GPT Image 2.5 | 없음 |

## 도구 조건

- YouArt 16:9 크기는 GPT Image 2.5 1088×608, Nano Banana Pro 1376×768, FLUX 2 1280×720, Midjourney 1456×816이다.
- YouArt 2:3 크기는 GPT Image 2.5 832×1248(custom), Nano Banana Pro 848×1264, Midjourney 896×1344다. FLUX 2는 2:3이 없어 3:4(768×1024)를 쓴다.
- 4장 기준 크레딧은 Nano Banana Pro 40, FLUX 2 56(노드 4개), Midjourney 10이다. GPT Image 2.5는 크기와 참조 수에 따라 12-32다.
- 실행 시간은 Midjourney를 뺀 묶음이 약 51-82초, Midjourney가 1분 17초에서 2분 30초였다.
-
- 원본이 WebP인 이미지는 LFS 규칙에 맞추려고 화질 손실 없이 PNG로 바꿔 저장했다. 이미지는 T1 영어만 저장소에 있다.

## 남은 일

1. T1과 T2 모두 도구마다 4장을 채우고 버전, 품질 설정, 크레딧을 기록한다.
2. GPT Image 2.5의 T1 한국어를 16:9로 다시 생성한다. 진행 조건을 어긴 실행을 바로잡는 것이므로 재시도로 보지 않는다.
3. GPT Image 2.5에만 시키지 않은 요소를 빼는 T1b를 만들어 실행한다. T1b 프롬프트는 아직 만들지 않았다.
4. T8 결과와 사용자 순위로 단계별 도구를 정한다.
5. GPT Image 2.5 3면도의 A 포즈와 견갑 위치를 보강하는 T5b를 만들지 정한다. 3면도에는 무기가 필요 없다. T5b 프롬프트는 아직 만들지 않았다.
6. 워크플로의 `T6 MJ v8.2` 노드에 결과가 없는 원인을 확인한다. T7 Midjourney 노드도 `service.generation_failed`로 실패했지만, 같은 프롬프트가 이미지 생성 화면에서는 생성됐다. T8 Midjourney 노드는 네 실행 중 세 실행이 `input.rejected`로 한 번 실패한 뒤 다시 실행해 생성됐다. 워크플로에서 실패하면 다시 실행하고, 계속 실패하면 이미지 생성 화면에서 따로 실행한다.
7. GPT Image 2.5의 T7 시트 4번과 3/4 시점 결과로 이미지→3D 단계를 진행한다.
8. GPT Image 2.5의 내용 번짐이 다른 종류의 참조로 줄어드는지 시험할지 정한다. 예를 들어 인물을 그릴 때 경비병이나 마을을 참조한다.
