# Jenova 자료 전체 목록과 용도 조사

[Jenova Resources](https://www.jenova.ai/en/resources)의 `Load More`를 끝까지 펼쳐 **글 2,947개 전체 목록**을 확보했다. 글을 20개 분야로 분류하고 제목·게시일·한국어 용도·원문 링크를 남겼다. 글에서 연결된 도구는 **공개 소개 페이지 433개**로 정리했다. 글 수와 도구 수는 서로 다르다.

## 바로 찾기

| 문서·데이터 | 읽는 때 |
|---|---|
| [도구별 용도](2026-11-04-jenova-resources/tools.md) | 서로 다른 도구 433개의 이름·용도·출처를 비교할 때 |
| [전체 목록 CSV](2026-11-04-jenova-resources/catalog.csv) | 글 2,947개를 제목·분야·용도·날짜로 검색하고 필터링할 때 |
| [열리지 않는 주소와 이전 주소](2026-11-04-jenova-resources/unavailable-links.md) | HTTP 404 주소 37개와 주소 이동 4개를 확인할 때 |
| [목록 원자료](2026-11-04-jenova-resources/catalog-source.json) | 목록의 원래 순서·제목·날짜·URL과 종료 상태를 대조할 때 |
| [글 조회·도구 연결 근거](2026-11-04-jenova-resources/article-links.json) | 글별 HTTP 상태·제목 일치·설명 유무·소제목 수·연결 도구를 확인할 때 |
| [도구 페이지 조회 근거](2026-11-04-jenova-resources/agent-pages.json) | 도구 주소별 응답 상태·최종 URL·페이지 제목을 확인할 때 |

CSV는 UTF-8 BOM 형식이다. `id`는 공개 목록 순서의 식별자이고 각 분야 문서의 ID와 같다. `all_agent_urls`에는 해당 글에 연결된 도구 주소 전체가 공백으로 구분되어 있다.

## 분야별 전체 글 목록

분류와 한국어 용도는 조사자의 요약이다. 각 글을 한 분야에만 넣어 전체 목록의 중복을 막았다. 여러 분야에 걸친 글은 제목과 본문 연결 도구를 기준으로 대표 분야에 배치했다. 같은 용도 요약을 가진 글이라도 원문의 세부 주제·게시일은 다를 수 있다.

| 분야별 목록 | 글 수 | 도구 수 | 읽는 때 |
|---|---:|---:|---|
| [세계관·지도](2026-11-04-jenova-resources/world-maps.md) | 36 | 6 | 세계·지리·문화·역사·지도 설정을 구상할 때 |
| [게임 설계·제작](2026-11-04-jenova-resources/game-design.md) | 93 | 5 | 전투·퀘스트·규칙·카드·퍼즐을 설계할 때 |
| [역할극·시뮬레이션](2026-11-04-jenova-resources/roleplay-games.md) | 422 | 10 | 대화형 모험·역할극·시뮬레이션을 찾을 때 |
| [캐릭터·동반자](2026-11-04-jenova-resources/characters.md) | 60 | 2 | 인물 설정·대화·동반자 활용을 찾을 때 |
| [소설·글쓰기·각본](2026-11-04-jenova-resources/writing.md) | 191 | 9 | 소설·대사·각본·시의 구상과 퇴고를 할 때 |
| [만화·애니메이션](2026-11-04-jenova-resources/comics.md) | 165 | 4 | 만화·웹툰·스토리보드의 장면과 컷을 구성할 때 |
| [이미지·그래픽·브랜드](2026-11-04-jenova-resources/art.md) | 118 | 26 | 이미지·로고·포스터·시각 자료를 만들 때 |
| [음악·공연](2026-11-04-jenova-resources/music.md) | 38 | 9 | 작곡·가사·공연·음악 학습 자료를 찾을 때 |
| [언어 학습·번역](2026-11-04-jenova-resources/languages.md) | 136 | 42 | 언어별 학습·회화·번역 도구를 비교할 때 |
| [학습·교육·연구](2026-11-04-jenova-resources/education.md) | 250 | 49 | 교과·시험·입시·학술 조사 자료를 찾을 때 |
| [개발·데이터·AI 도구](2026-11-04-jenova-resources/tech.md) | 215 | 34 | 코딩·데이터·검색·MCP·AI 플랫폼을 다룰 때 |
| [문서·업무 생산성](2026-11-04-jenova-resources/documents.md) | 116 | 12 | 문서·표·발표·메일·업무 정리를 할 때 |
| [사업·마케팅·커리어](2026-11-04-jenova-resources/business.md) | 186 | 41 | 사업·홍보·판매·인사·취업을 준비할 때 |
| [재무·투자·시장](2026-11-04-jenova-resources/finance.md) | 203 | 25 | 재무·시장·투자 자료의 분석 용도를 찾을 때 |
| [건강·운동·심리](2026-11-04-jenova-resources/health.md) | 129 | 24 | 건강·운동·영양·심리 관련 정보 용도를 찾을 때 |
| [관계·가정·자기계발](2026-11-04-jenova-resources/relationships.md) | 91 | 14 | 관계·가정·대화·생활 습관을 정리할 때 |
| [점술·종교·성찰](2026-11-04-jenova-resources/divination.md) | 207 | 33 | 점술·상징 해석·종교 문헌을 탐색할 때 |
| [여행·음식·생활·취미](2026-11-04-jenova-resources/lifestyle.md) | 245 | 79 | 여행·음식·패션·주거·스포츠·취미를 찾을 때 |
| [법률·정책·시사](2026-11-04-jenova-resources/law-politics.md) | 45 | 9 | 법률·계약·이민·정치·국방 자료를 찾을 때 |
| [플랫폼·기타](2026-11-04-jenova-resources/other.md) | 1 | 0 | 위 분야에 포함되지 않는 글을 찾을 때 |

합계는 글 2,947개, 도구 433개다. 글과 도구의 분야는 각각 분류했으므로 한 분야의 글에서 다른 분야 도구가 연결될 수 있다. 분야별 문서의 첫 연결 도구는 본문에서 추출한 링크 순서이며 공식 대표 도구라는 뜻은 아니다.

## 세계관·시나리오 작업에서 먼저 볼 자료

아래는 현재 작업에 맞춘 활용 제안이다. 이 조사에서 새 스킬을 만들거나 도구를 실행한 것은 아니다.

| 도구 | 구체적으로 맡길 일 | 연결되는 결과 |
|---|---|---|
| [Worldbuilding Studio](https://www.jenova.ai/a/worldbuilding-studio) | 세계의 규칙·자원·문화·세력 이해관계 연결 | 도시·세력 갈등의 원인과 영향 |
| [Map Maker](https://www.jenova.ai/a/map-maker) | 세계·지역·정착지·던전의 지도 구상과 기존 배치 비평 | 지형·이동 경로·공간 관계 |
| [Game Designer Assistant](https://www.jenova.ai/a/game-designer-assistant) | 전투·보스·퀘스트·성장 구조의 조건 정리 | 사건과 전투가 이어지는 플롯 조건 |
| [Character Creator](https://www.jenova.ai/a/character-creator) | 주인공·동료·적의 동기·결핍·관계 구성 | 선택·갈등·관계 변화 |
| [Creative Fiction Writer](https://www.jenova.ai/a/creative-fiction-writer) | 사건의 인과관계·장면·대사·복선 구성 | 지역별 이야기 흐름 |
| [Mythology Creator](https://www.jenova.ai/a/mythology-creator) | 신화·괴물의 기원·신계·전승 구성 | 몬스터·보스와 세계의 역사 연결 |
| [Religion Creator](https://www.jenova.ai/a/religion-creator) | 교리·의례·금기·종교 공동체 구상 | 세력의 가치관·갈등·생활 문화 |
| [Conlang Creator](https://www.jenova.ai/a/conlang-creator) | 지명·인명·고대어에 쓸 언어 체계 구성 | 문화권별 명명 규칙 |
| [Concept Art Creator](https://www.jenova.ai/a/concept-art-creator) | 캐릭터·환경·괴물의 시각 콘셉트 구상 | 설정을 확인할 시각 자료 |

현재처럼 플롯을 먼저 정할 때는 세계관·캐릭터·글쓰기 도구의 사고 절차를 참고하고, 지도나 이미지는 공간 관계를 설명해야 할 때 붙이는 방향을 제안한다. 모든 소개 도구를 도입할 필요는 없다. 도구 수를 늘리면 같은 설정을 여러 곳에 입력하고 결과를 맞추는 비용도 늘어난다.

## 수집 범위와 종료 근거

2026-11-04 공개 목록에서 `Load More`를 반복 클릭했다. 첫 화면의 12개가 2,947개까지 늘어난 뒤 버튼과 다음 페이지 링크가 사라졌다. 마지막 상태는 [목록 원자료](2026-11-04-jenova-resources/catalog-source.json)의 `next: null`이다. 페이지당 12개 기준 246페이지 분량이며 마지막은 7개다. 이는 누적 개수의 환산이다.

| 확인 항목 | 결과 | 근거 |
|---|---|---|
| 목록 항목·고유 URL | 각각 2,947개 | 목록 원자료 |
| URL 중복·빈 제목·빈 날짜 | 각각 0개 | 목록 원자료 대조 |
| 게시일 범위 | 2025-07-12부터 2026-10-24까지 | 목록의 표시 날짜 |
| 첫 항목 | AI RPG Token Generator: Custom Hero, NPC & Monster Art for Your VTT | [첫 글](https://www.jenova.ai/en/resources/ai-rpg-token-generator) |
| 마지막 항목 | Local vs Remote MCP Servers: AI Infrastructure Guide | [마지막 글](https://www.jenova.ai/en/resources/local-vs-remote-mcp-servers-a-strategic-guide) |
| 글 HTML 조회 | 2,947개 모두 HTTP 200 | 글 조회·도구 연결 근거 |
| 목록 제목과 H1 일치 | 2,947개 | 글 조회·도구 연결 근거 |
| 설명 메타데이터 존재 | 2,947개 | 글 조회·도구 연결 근거 |
| 본문에 `/a/` 링크가 없는 글 | 43개 | 글 조회·도구 연결 근거 |
| 정규화한 서로 다른 `/a/` 주소 | 474개 | 글 연결 주소 합집합·도구 조회 근거 |
| 도구 주소 응답 | HTTP 200 437개, HTTP 404 37개 | 도구 페이지 조회 근거 |
| 같은 최종 페이지로 이동한 이전 주소 | 4개 | 열리지 않는 주소와 이전 주소 |
| 서로 다른 최종 성공 소개 페이지 | 433개 | 최종 URL 중복 제거 |

[앞선 지도 조사](2026-11-01-jenova-map-generation.md)는 2,376개까지 확인한 부분 조사다. 이 문서는 별도로 목록 끝을 확인한 전체 조사이며, 앞선 기록의 조사 범위를 소급해서 바꾸지 않는다.

## 용도 조사 방법과 확인 한계

모든 글의 HTML을 조회하고 H1, 설명 메타데이터, 첫 다섯 개까지의 H2·H3, 본문 도구 링크를 추출했다. 연결된 도구의 공개 소개 페이지도 조회해 이름·설명을 확인했다. 이를 바탕으로 한국어 용도를 정리했다. 2,947개 글의 본문 문장을 모두 정독하거나 비교·성능 주장을 독립 검증한 것은 아니다.

글 제목의 `Best`, `Free`, 성능·효과 표현은 업체가 붙인 제목을 그대로 보존한 것이다. 추천 순위·무료 제공·성과를 이번 조사 결과로 보증하지 않는다. HTTP 200은 공개 소개 페이지 조회 성공이며 로그인 이후 도구 실행·현재 요금·결과 품질의 검증과 다르다. 실제 실행·결제·프로젝트 자료 업로드는 하지 않았다.

전체 목록의 범위는 위 공개 Resources 페이지에서 끝까지 펼쳐 확인한 항목이다. Jenova 전체 서비스나 비공개 자료의 완전한 목록이라는 뜻은 아니다. 원문의 긴 본문은 복제하지 않고 제목·출처·조회 근거와 한국어 용도 요약을 남겼다.
