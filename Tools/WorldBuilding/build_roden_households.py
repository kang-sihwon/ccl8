"""로덴의 나머지 상주민을 재현 가능한 정적 HTML 가구 대장으로 편찬한다."""
import json
from pathlib import Path
import random
import re

ROOT = Path(__file__).resolve().parents[2]
DRAFT = ROOT / "Docs/Narrative/Drafts/2026-10-31-ether-taiga-dawn"
OUT = DRAFT / "roden-households"
DETAIL = DRAFT / "2026-11-28-roden-lives.html"
VOLUME = 60

# 인구와 상세 인물은 사고 전날의 거주지로 센다.
DISTRICTS = (
    ("M", "안쪽 식료 시장과 주택가", 10400, 11, "화덕길 콩전골목 안우물길 종루아랫길 채소전길 소금고개 맷돌길 마른장터길", "food food cloth home ledger field pottery timber paper goods", (27, 27, 41, 5)),
    ("G", "조합·부품 시장과 공방 골목", 7200, 12, "놋쇠길 줄장이골목 물레길 톱밥마당 수레채길 풀무고개 돌확길 포장천길", "metal metal timber gear cloth pottery ledger home paper goods", (27, 51, 19, 3)),
    ("Q", "제방 안쪽 부두 노동자 동네", 4800, 12, "밧줄골목 북잔교길 나룻등길 소금턱길 모랫길 통장이길 차양길 닻마당", "river river freight fish cord field timber food home goods", (65, 20, 13, 2)),
    ("Y", "작업장 외곽 집터와 작업 마당", 1600, 11, "기름등길 바깥우물길 교대마당 벽돌길 자갈턱길 솔마당길 낮은담길 회토길", "gear stone metal home field freight cloth food timber cord", (58, 30, 11, 1)),
)
HOMES = ("공동벽을 낀 셋집", "작업채 딸린 집", "아래층을 가게로 쓰는 집", "안뜰과 곳간을 둔 집")
# 각 생업군에서 실제로 겪을 법한 물건 세 가지, 과거의 경험 세 가지와 현재의 선택을 함께 둔다.
TRADES = {
"food": ("빵장이|두부 장수|곡물 소매상|맥아 말리는 일꾼|국 장수", "빈 됫박|나무 쟁반|보릿단", "장날 비를 맞으며 남은 먹을거리를 이웃과 나눈 날부터 마감 때 한 사람 몫을 따로 챙긴다.|새벽 손님이 선 채로 먹는 걸 보고 가게 앞에 앉을 판을 놓았다.|장날이 밀려 식재료가 남은 주에 새로 조리하는 법을 배워 한겨울에도 써먹는다."),
"cloth": ("재단사|염색 직인|실 잣는 일꾼|작업복 수선공|베 짜는 사람", "물레 가락|삼실 타래|덧감 한 폭", "헌 소매를 뜯다가 손목 안쪽에 숨긴 수를 찾았다. 이후 남길 바느질 자국부터 주인에게 묻는다.|한 집의 옷 세 벌을 한꺼번에 맡았다가 치수가 서로 달라 천 조각에 손가락 너비를 표시하기 시작했다.|장터에서 볕과 처마 그늘의 천 빛깔이 다른 걸 보고 천을 건넬 때 문밖까지 함께 나간다."),
"home": ("빨래 품팔이|물 긷는 일꾼|심지 꼬는 사람|마당 청소 일꾼|공동 목욕간 일꾼", "물동이 고리|등잔 접시|빨랫방망이", "겨울 새벽에 손님 집 문이 얼어 한참 밖에 서 있었다. 이후 첫 집과는 전날 저녁에 만날 곳을 정한다.|시장 우물 차례를 바꿔 준 이웃에게 빈손으로 갈 수 없어 퇴근할 때 물 한 통을 더 져 나른다.|햇볕 드는 처마를 서로 빌려 쓰던 때 알게 된 집이 지금까지 첫 단골이다."),
"ledger": ("장부 필경사|품표 서기|창고 계수원|주문서 대필인", "접어 둔 차용표|납작한 먹돌|나무 셈패", "날짜를 잘못 읽은 손님과 빈 가판 앞에서 기다린 뒤로는 장날을 날짜 옆에 그림으로도 적어 준다.|글 모르는 단골이 자기 이름을 찾아 짚는 순간을 보고 서명할 칸을 종이 아래 따로 둔다.|인수표를 집에 가져가 확인해 준 일이 계기가 되어 저녁 교대자 몫의 사본을 한 장 더 만든다."),
"field": ("텃밭 소작인|약초 다듬는 사람|채소 장수|가금 돌보는 일꾼|종자 고르는 사람", "버들 광주리|씨앗 주머니|짚 깔개", "늦서리 든 아침에 이웃과 살릴 것을 골랐다. 지금도 찬바람이 오면 담 너머까지 먼저 살핀다.|빈 공터에서 시작한 장날 좌판이 잘되어 그때 자리를 나눠 준 이에게 해마다 첫 수확을 건넨다.|물길 아래쪽 집에서 한 철 도운 뒤론 물을 받아 쓴 날에는 돌아가는 도랑도 보고 온다."),
"pottery": ("옹기 직인|가마 불지기|기와 고르는 사람|흙반죽 일꾼", "초벌 사발|굽 낮은 단지|가마 받침", "가마 맨 앞에 놓아 유난히 검게 난 그릇을 손님이 골랐다. 그 뒤로 한 가지 빛만 골라 내놓지 않는다.|깨진 그릇의 금을 손끝으로 찾는 법을 장터의 늙은 손님에게 배워 지금도 물건을 넘길 때 함께 만져 본다.|비 온 뒤 흙이 마르길 기다리며 이웃 가마를 도왔다. 이젠 불 땔 날이 겹치면 서로 몫을 나눠 싣는다."),
"timber": ("바퀴 목수|통장이|널판 재단공|상자 목수|가구 수선공", "자작나무 쐐기|닳은 대팻날|굽은 자", "납품 길에 문폭을 잘못 재서 완성품을 다시 풀었다. 그 뒤로 의뢰받으면 완성품이 지날 문부터 걷는다.|선반을 짠 뒤 주인이 직접 물건을 올리는 걸 보고 한 뼘 낮춰 줬다. 그 집 아이가 자라 다시 높여 달라 온다.|나뭇결을 거슬러 판재가 갈라진 날 버릴 조각으로 작은 함을 만들었다. 지금도 견습에게 가장 작은 조각을 고르게 한다."),
"paper": ("종이 뜨는 직인|책 매는 사람|봉함지 장수|가게 표찰 필사공", "먹 묻은 천|얇은 나무판|접지 자", "수레에 실은 종이가 젖은 날 글줄만 건져 말려 준 인연으로 부두 쪽 주문을 계속 받는다.|어린 손님이 펼치지 못한 묶음을 풀어 주다가 묶는 방향을 바꿨다. 새 주문도 건네기 전에 직접 열어 본다.|남은 종이 가장자리를 잘라 심부름꾼들의 주소 쪽지로 나눴더니 길을 묻던 이들이 단골이 됐다."),
"goods": ("헌옷 중개상|식료 도매상|기름 소매상|장터 행상|소금 거래인", "황동 추|접는 좌판|거스름돈 주머니", "큰 묶음을 사 간 집 앞에서 이웃 셋이 다시 나누는 걸 보았다. 지금은 작은 몫도 따로 달아 준다.|봄장이 서지 않은 주에 묵은 빚을 품으로 받은 인연이 남아 겨울 물량은 그 집과 먼저 의논한다.|가게 그림자가 옮겨 가는 오후엔 물건 빛이 달라 보여 손님이 고른 것을 길 한복판에서도 보여 준다."),
"metal": ("부속 줄장이|장식못 직인|놋그릇 수선공|쇠테 대장장이|경첩 직인", "작은 모루|마모된 고리|손줄 자루", "버린 부속을 집에 가져간 아이가 소리를 맞춰 놀았다. 지금도 헌 쇠를 골라 울림이 좋은 조각은 따로 모은다.|다 손봤다고 돌려준 물건이 원래 자리에 맞지 않아 그 집에서 하루를 보냈다. 이젠 닳은 쌍도 함께 받아 본다.|손잡이의 오래된 홈을 무심코 매웠다가 주인이 손에 익던 자리를 찾는 걸 보고 다음 수리부터 먼저 묻는다."),
"gear": ("수동문 정비공|배관 피복공|도르래 손질공|난방 설비 직인|점검구 청소공", "헌 피복 조각|참나무 손잡이|교대 표찰", "주간조와 같은 소리를 서로 다르게 부른 걸 알고 아침 교대에 남아 들은 자리부터 같이 짚었다.|혼자 들던 공구함을 다음 교대자가 못 열자 안 걸리는 고리로 바꿨다. 새로 온 사람에게는 맨 먼저 그 함을 건넨다.|한겨울에 이웃집을 돌며 유난히 찬 방을 찾아본 뒤부터 도면 귀퉁이에 실제로 손을 댄 자리도 그려 둔다."),
"river": ("강배 노잡이|나룻배 사공|갑판 일꾼|잔교 보조원|도선 연락원", "노걸이 고리|방수 덮개|젖은 줄 끝", "안개에 선착장을 지나친 날 강가 화덕 냄새를 따라 자리를 찾았다. 요즘도 물길을 알려 줄 때 냄새까지 일러 준다.|빈 배를 기다리는 아이와 한나절을 보낸 뒤로 하역이 길어지면 먼저 육지 집에 소식을 보낸다.|배가 낮게 뜨는 철에 어린 승객이 뭍을 못 딛는 걸 보고 접안 전 맨 뒤 사람의 발도 살핀다."),
"freight": ("부두 짐꾼|곡물 포대 운반공|수레몰이꾼|창고 인수원|시장 배달꾼", "밧줄 등받이|나무 받침목|빈 짐바구니", "짐에 가려 뒷사람을 놓친 날부터 출발 전에 짝이 선 쪽을 눈으로 확인한다.|배가 늦어 밤늦게 짐을 부린 겨울에 옆집이 국 한 그릇을 남겨 줬다. 늦는 교대자의 그릇을 챙기는 버릇이 생겼다.|받는 집이 바뀐 걸 몰라 빈 골목까지 돌아간 뒤로는 인수받은 이의 이름을 돌아와서 한 번 더 말해 둔다."),
"fish": ("어망 수선공|강 어부|생선 염장공|물고기 소매상", "갈대 바구니|그물추|작은 소금독", "물이 얕아진 여름에 조그만 고기를 도로 놓아주던 아이를 따라 강 굽이를 걸었다. 그 뒤론 계절마다 물가 높이를 기억한다.|밤배만 오던 철에 소금을 나누어 쓴 이웃과 지금도 첫 장 물건을 나눠 먹는다.|젖은 바구니의 바닥이 빠진 날 한 마리씩 주워 준 이들에게 장 끝에 남은 것을 돌려 보냈다. 이듬해엔 그 집들이 먼저 찾아왔다."),
"cord": ("밧줄 꼬는 직인|차양 수선공|돛천 기움장이|그물실 장수", "삼끈 꾸러미|가죽 골무|돛천 귀퉁이", "물 먹은 줄이 달라붙어 매듭을 못 풀던 날 사공에게 손끝으로 끝을 찾는 법을 배웠다. 건넬 때마다 함께 한 번 풀어 본다.|헌 차양의 한 귀를 주인이 기념으로 찾아간 뒤로 손보기 전 남기고 싶은 자리가 있는지부터 묻는다.|뒤뜰의 줄이 마르지 않은 겨울 이웃 처마를 빌렸다. 봄부터는 마당에 남의 줄 한 가닥 걸 자리를 비워 둔다."),
"stone": ("벽돌 굽는 사람|돌계단 수선공|배수로 석공|회반죽 미장이", "벽돌 반 장|수평 보는 물그릇|작은 돌쐐기", "문턱을 반듯하게 고쳤는데 물이 마당에 괴었다. 다음 비에 돌아와 보고 지금도 비 온 다음 날 일한 자리를 찾는다.|무거운 돌을 나르던 중 이웃이 남긴 판자 덕에 수레가 들어왔다. 요즘도 좁은 일터에는 남의 수레 나갈 길을 먼저 낸다.|손도장이 남은 벽돌을 안쪽으로 돌리자 주인이 도로 보여 달라 했다. 그 뒤로 쓸 만한 흔적은 주인 손으로 고르게 한다."),
}
FIRST = "아 에 이 오 우 가 게 고 나 네 노 누 다 데 도 라 레 로 루 리 마 메 모 미 바 베 보 비 사 세 소 시 수 타 테 토 티 하 헤 호 키 카 클 브 프 드 벨 갈 솔 밀 델 젠 알 엘 이스".split()
LAST = "나 니 누 네 다 덴 도 라 란 람 렌 레 로 르 리 린 마 메 모 미 바 베 보 빈 사 산 세 셀 소 스 시 아 안 에 엘 오 온 우 운 이 인 카 케 코 타 테 토 트 파 페 하 헬".split()
BUILDS = "길쭉한 얼굴|둥근 턱|넓은 미간|도드라진 광대|달걀형 얼굴|끝이 들린 코|도톰한 아랫입술|낮고 넓은 콧등|긴 속눈썹|깊은 눈매|겹이 얇은 눈꺼풀|넓은 이마|갈라진 턱끝|볼우물 하나|비스듬히 솟은 눈썹|잔잔한 주근깨|작은 귓불|선명한 입술선".split("|")
HAIR = "짙은 갈색|검은색|황갈색|옅은 밤색|적갈색|짙은 잿빛".split("|")
HAIR_STYLES = "가르마를 깊게 낸|목덜미가 보이게 자른|귀를 살짝 덮는|정수리에 가마가 둘인|빗어도 끝이 뻗치는|가늘게 곱슬거리는|매끈하게 뒤로 넘긴|이마 한가운데로 갈라지는".split("|")
OBJECTS = "작은 나무 사발|기운 무릎담요|물빛 유리구슬|짚으로 엮은 새|버드나무 숟가락|접어 만든 종이배|꽃 자수를 놓은 띠|납작한 강돌|매끈한 실패|조개껍데기 목걸이|붉은 실타래|물고기 모양 단추|볕에 바랜 모자|단풍잎 본뜬 목판|자작껍질 상자|황동 방울|닳은 뿔빗|가죽 동전집|작은 물피리|납작한 차 받침".split("|")
SEASONS = "갈대가 하얗게 피던 가을|큰 장이 두 번 연달아 선 여름|손 빠른 이도 장갑을 끼던 겨울|처마의 고드름이 풀리던 봄|낮배가 늦던 가을|뜰마다 햇곡식을 널던 초가을|밤안개가 짙던 늦봄|빗물이 사흘간 고였던 여름|마른 바람에 차양이 울리던 봄|강물이 일찍 언 겨울".split("|")
PLACES = "제방 돌벤치|빨랫줄 아래|공동 화덕 문간|나루로 난 내리막|우물 옆 낮은 담|장터 뒷골목|곡물 창고 처마|교대 길 첫 모퉁이|부속 시장 계단|종루 아래 쉼터|옹기전 평상|널판 가게 문턱".split("|")
SKILLS = "어둠 속에서 매듭 풀기|작은 솥의 물 넘치기 전에 뚜껑 열기|물동이 한 쌍의 무게 맞추기|구운 빵 소리로 속이 익었는지 가리기|손으로 물새 울음 흉내 내기|서로 다른 두 실 한 가닥으로 잇기|비 오는 날 젖지 않게 종이 싸기|부러진 바구니 손잡이 감기|비탈에서 작은 통 곧게 굴리기|가죽에 난 구멍 같은 간격으로 꿰기|갈대로 풀피리 불기|콩 한 줌으로 두 사람 몫 세기".split("|")
FOODS = "구운 밤|보리빵|콩죽|양파 수프|절인 순무|밤꿀 과자|삶은 달걀|말린 사과|치즈 한 조각|햇완두|따뜻한 맥아죽|자두 졸임".split("|")
EARLY = "손수건 귀퉁이에 매듭을 지어 두었다|빈 사발 둘을 포개 창가에 올렸다|길 높이와 눈높이가 달라지는 턱에 앉아 있었다|배 이름을 불러 가며 마룻바닥에 금을 그었다|작은 통으로 화덕의 울림을 흉내 냈다|이웃집 문에 처음 혼자 인사를 했다|긴 풀잎을 접어 고리 두 개를 이었다|집으로 오는 골목 문 색깔을 외웠다".split("|")
INFANT = "빛이 옮겨 가면 눈동자부터 천천히 따라간다|방울 소리에 고개를 틀고 익숙한 손가락을 꼭 쥔다|따뜻한 천이 볼을 덮으면 오므렸던 손을 편다|마룻장이 삐걱일 때마다 소리 나는 쪽으로 눈을 뜬다|가까이서 낮은 목소리가 들리면 입술을 달싹인다|빨랫줄 그림자가 천장을 지나갈 때 눈길을 오래 둔다".split("|")
PASTIMES = "마른 잎을 눌러 모은다|빈 실패로 굴림말을 만든다|장날 들은 노래를 흥얼거린다|둥근 자갈에 물새를 그린다|작은 천 조각을 이어 방석을 깁는다|해마다 첫 꽃이 피는 날을 세어 둔다|마당에 오는 새의 발자국을 그려 둔다|부러진 갈대로 바람개비를 만든다|잠들기 전에 그날 들은 우스운 말을 되살린다|헌 종이에 굽이마다 다른 강 모양을 그린다|마른 콩으로 홀짝을 맞히며 논다|나뭇잎 모양으로 종이를 오린다".split("|")
KIN = "이웃집 어른|어릴 적 벗|옛집 건넛방 사람|자주 마주치던 뱃사공|골목 끝 바느질장이|장터의 떡 장수|같이 자란 사촌|이웃집 노인|우물가 단골|예전 집주인".split("|")


def particle(word, pair):
    """받침에 맞춰 조사를 붙인다."""
    return word + pair[0 if (ord(word[-1]) - 0xAC00) % 28 else 1]


def makeup(rng, size):
    sex = lambda: rng.choice(("여성", "남성"))
    if size == 1:
        return [("단독 가구원", sex(), rng.randint(19, 85))]
    if rng.random() < (0.24 if size < 5 else 0.08):
        ages = rng.sample(range(19, 67), size)
        if size == 2 and rng.random() < 0.45:
            age = rng.randint(23, 69)
            return [("손위 형제자매", sex(), age), ("동생", sex(), age - rng.randint(2, 10))]
        return [("공동 살림 대표" if k == 0 else "동거 일꾼", sex(), age) for k, age in enumerate(ages)]
    solo_parent = rng.random() < 0.24
    adult_age = rng.randint(26, 55) if solo_parent or size > 2 else rng.randint(23, 77)
    partner_age = max(22, adult_age + rng.randint(-4, 5))
    members = [("부모" if solo_parent or size > 2 else "배우자", sex(), adult_age)]
    if not solo_parent:
        members.append(("배우자", "남성" if members[0][1] == "여성" else "여성", partner_age))
    if size >= 4 and rng.random() < 0.45:
        members.append(("첫 가구원의 부모", sex(), adult_age + rng.randint(20, 32)))
    slots = size - len(members)
    if slots:
        youngest_parent = adult_age if solo_parent else min(adult_age, partner_age)
        oldest_parent = adult_age if solo_parent else max(adult_age, partner_age)
        ages = range(max(0, oldest_parent - 45), youngest_parent - 17)
        for age in sorted(rng.sample(list(ages), slots), reverse=True):
            members.append(("자녀", sex(), age))
    return members


def occupation(rng, age, sector, wealthy):
    if age < 6:
        return "가구에서 돌보는 영유아"
    if age < 12:
        return rng.choice(("글을 배우는 아이", "셈을 배우는 아이", "집안 심부름을 배우는 아이"))
    jobs = TRADES[sector][0].split("|")
    if age < 16:
        return rng.choice(("가게 심부름", "집안일을 익히는 청소년", "글과 셈을 배우는 청소년"))
    job = rng.choice(jobs)
    if age < 20:
        return job + " 수습"
    if age >= 66 and rng.random() < 0.7:
        return "은퇴한 " + job
    if wealthy and sector == "goods":
        return rng.choice(("도매 상단 운영자", "소금 창고 임대인", "식료 도매상"))
    return job


def appearance(rng, age):
    feature = rng.choice(BUILDS)
    color = rng.choice(HAIR if age < 55 else HAIR + ["희끗한 잿빛", "은백색", "백발에 가까운 회색"])
    if age < 2:
        return f"{feature}. {color} 솜털이 귀 언저리에 가늘게 남았다."
    marks = ("걸음을 옮길 때 발끝이 안으로 살짝 모인다.", "미소 지으면 볼 한쪽이 먼저 올라간다.", "사람을 부를 때 고개를 옆으로 기울인다.", "눈을 가늘게 뜨면 코끝이 따라 움직인다.", "어깨가 둥글고 손가락이 길다.", "왼쪽 눈썹 끝에 작은 점이 있다.", "웃을 때 귓불까지 붉어진다.", "눈을 동그랗게 뜨고 상대의 입을 바라본다.")
    return f"{feature}. {rng.choice(HAIR_STYLES)} {color} 머리. {rng.choice(marks)}"


def personal_history(rng, age, sector, neighbor, lane):
    obj = rng.choice(OBJECTS)
    thing = particle(obj, "을를")
    when, place = rng.choice(SEASONS), rng.choice(PLACES)
    person = particle(neighbor, "은는")
    skill, hobby = rng.choice(SKILLS), rng.choice(PASTIMES)
    if age == 0:
        return f"태어난 뒤 처음 집으로 오던 날 {person} {thing} 요람 곁에 놓았다. 요즘은 {rng.choice(INFANT)}."
    if age < 4:
        return f"얼마 전 {person} {thing} 창가에 놓아 주었다. 요즘은 {rng.choice(INFANT)}."
    if age < 6:
        return f"지난 장날 {place}에서 {thing} 놓고 돌아왔다. {person} 아이와 그 길을 되짚어 함께 찾아왔다. 지금은 잠들기 전에 제 자리에 있는지 손을 뻗어 본다."
    if age < 12:
        return f"{rng.randint(5, age - 1)}살 때 {place}에서 {thing} 품에 안고 {rng.choice(EARLY)}. 지금은 {hobby}."
    if age < 16:
        return f"{rng.randint(6, age - 2)}살 때 {place}에서 {particle(rng.choice(KIN), '이가')} 가르쳐 준 놀이를 아직 기억한다. 요즘은 {particle(skill, '을를')} 혼자 연습한다. 쉬는 날에는 {hobby}."
    year = rng.randint(14, max(14, age - 2))
    item = rng.choice(TRADES[sector][1].split("|"))
    memory = rng.choice(TRADES[sector][2].split("|"))
    food = rng.choice(FOODS)
    mentor = rng.choice(KIN)
    v = rng.randrange(9)
    if v == 0:
        return f"{year}살 무렵 {when}의 일이다. {memory} 집에 두고 아끼는 것은 그 시절부터 간직한 {particle(obj, ('이다', '다'))}."
    if v == 1:
        return f"{year}살 때 {place}에서 {particle(item, '을를')} 챙겨 준 {mentor} 덕에 첫 약속에 늦지 않았다. 그날 나눠 먹은 {food} 맛이 그리워 장날마다 그 길로 돌아온다."
    if v == 2:
        broken = rng.choice(("옷상자 경첩", "빠진 서랍 고리", "의자 다리", "나간 문빗장", "등잔 덮개", "낡은 맷방석", "실패 꽂이", "깨진 물동이 손잡이"))
        return f"{year}살에 {lane}에서 처음 {particle(broken, '을를')} 손수 고쳤다. 그 일을 본 {mentor}에게서 {particle(item, '을를')} 선물 받아 지금도 손 닿는 데 둔다."
    if v == 3:
        return f"{year}살 되던 해 {place}에서 {skill} 내기를 했다가 점심 몫 {particle(food, '을를')} 내줬다. 겨울에 다시 만나 겨뤄 보자는 약속 때문에 쉬는 날마다 연습한다."
    if v == 4:
        return f"{year}살에 {when}을 보내며 {particle(item, '을를')} 빌려 준 집 일을 며칠 도왔다. 이제는 그날 알게 된 {mentor}의 생일마다 {particle(food, '을를')} 싸 들고 간다."
    if v == 5:
        return f"{year}살 무렵 {place}에서 일감을 처음 혼자 기다렸다. 돌아오는 길에 {thing} 구해 첫 품의 기억으로 남겼다. 쉴 때는 {hobby}."
    if v == 6:
        return f"{year}살에 {when} 장이 일찍 파해 {particle(food, '을를')} 싸 들고 {place}까지 걸었다. 거기서 {mentor}에게서 {particle(skill, '을를')} 배웠다. 지금은 집에 손님이 오면 먼저 보여 준다."
    if v == 7:
        return f"{year}살 때 {place}에서 {particle(mentor, '과와')} 함께 묵은 짐을 정리하다 {thing} 찾아 주었다. 사례로 받은 {particle(food, '을를')} 집에 나눠 온 뒤부터 해마다 같은 철에 그 집을 찾는다."
    return f"{year}살에 처음 {particle(item, '을를')} 맡아 보던 철이었다. {memory} 요즘 저녁에는 {hobby}."
def make_district(spec, named, histories):
    key, label, total, detailed, streets, sectors, weights = spec
    rng = random.Random("Roden-2026-11-28-" + key)
    houses = []
    remaining = total - detailed
    while remaining:
        size = min(remaining, rng.choices(range(1, 8), (10, 16, 21, 24, 17, 9, 3))[0])
        number = len(houses) + 1
        hid = f"{key}{number:04d}"
        street = rng.choice(streets.split())
        home = rng.choices(HOMES, weights)[0]
        makeup_rows = makeup(rng, size)
        names = []
        for _ in makeup_rows:
            while True:
                name = rng.choice(FIRST) + rng.choice(LAST)
                if rng.random() < 0.22:
                    name += rng.choice(("나", "엘", "라", "르", "안", "아", "네", "온"))
                if name not in named and name not in names:
                    names.append(name)
                    break
        house_trade = rng.choice(sectors.split())
        if home == HOMES[3] and rng.random() < 0.6:
            house_trade = rng.choice(("goods", "ledger", "timber"))
        members = []
        for i, (relation, sex, age) in enumerate(makeup_rows):
            sector = house_trade if i == 0 or rng.random() < 0.52 else rng.choice(sectors.split())
            others = [names[j] for j, (_, _, years) in enumerate(makeup_rows) if j != i and years >= 18]
            neighbor = rng.choice(others) if others else "이웃 어른"
            job = occupation(rng, age, sector, home == HOMES[3])
            for attempt in range(200):
                history = personal_history(rng, age, sector, neighbor, street)
                if history not in histories:
                    break
            else:
                raise ValueError(f"{hid}-{i+1}: 개인사 조합 부족")
            histories.add(history)
            members.append((names[i], relation, sex, age, job, appearance(rng, age), history))
        houses.append((hid, street, home, members))
        remaining -= size
    return houses


PAGE_CSS = """body{max-width:58rem;margin:0 auto;padding:1.2rem;color:#2b2926;background:#fffdf8;font:1rem/1.7 system-ui,sans-serif;overflow-wrap:anywhere}a{color:#155346}h1,h2,h3{line-height:1.35}section{margin:2rem 0}article{border-top:1px solid #b5aa94;padding:.4rem 0}h3{margin-bottom:.25rem}p{margin:.35rem 0 .7rem}nav a{margin-right:1rem}li{margin:.3rem 0}.meta{color:#5a5042}input{padding:.5rem;max-width:95%;font:inherit}details{margin:.8rem 0}summary{cursor:pointer}"""
def page(title, body):
    return f'<!doctype html>\n<html lang="ko"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>{title}</title><style>{PAGE_CSS}</style></head><body><main>{body}</main></body></html>\n'


def volume_name(key, number):
    return f"{key.lower()}-{number:02d}.html"


def render_volume(key, district, number, volumes, homes, search):
    file = volume_name(key, number)
    nav = ['<a href="index.html">가구 명부 검색</a>']
    if number > 1:
        nav.append(f'<a href="{volume_name(key, number-1)}">앞 권</a>')
    if number < volumes:
        nav.append(f'<a href="{volume_name(key, number+1)}">다음 권</a>')
    navigation = "<nav>" + " ".join(nav) + "</nav>"
    title = f"{district} · 가구 대장 {number}권"
    body = [navigation, f"<h1>{title}</h1>", f"<p>{homes[0][0]}부터 {homes[-1][0]}까지 {len(homes)}가구.</p>"]
    for hid, street, home, members in homes:
        body.append(f'<section id="{hid}"><h2>{hid} · {street}</h2><p>{home} · {len(members)}명</p>')
        for idx, (name, relation, sex, age, job, looks, history) in enumerate(members, 1):
            pid = f"{hid}-{idx}"
            body.append(f'<article id="{pid}"><h3>{name}</h3><p class="meta">{relation} · {sex} · {age}세 · {job}</p><p>{looks}</p><p>{history}</p></article>')
            search.append([name, pid, job, street, file])
        body.append("</section>")
    body.append(navigation)
    (OUT / file).write_text(page(title, "\n".join(body)), encoding="utf-8")


SEARCH_JS = """<script>
const people=JSON.parse(document.getElementById('names').textContent);
const input=document.getElementById('query'), list=document.getElementById('matches'), note=document.getElementById('count');
input.addEventListener('input',()=>{
  const q=input.value.trim().toLowerCase();
  list.replaceChildren();note.textContent='';
  if(!q)return;
  const hits=people.filter(row=>row.slice(0,4).some(x=>x.toLowerCase().includes(q)));
  note.textContent=hits.length+'명'+(hits.length>60?' · 처음 60명을 표시해. 가구 번호로 더 좁힐 수 있어.':'');
  for(const [name,id,job,street,file] of hits.slice(0,60)){
    const li=document.createElement('li'), link=document.createElement('a');
    link.href=file+'#'+id;link.textContent=name+' · '+job+' · '+street+' · '+id;
    li.append(link);list.append(li);
  }
});</script>"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    named = re.findall(r'<article id="npc-([^"]+)">', DETAIL.read_text(encoding="utf-8"))
    assert len(set(named)) == len(named) == 47
    search, sections = [], []
    households = 0
    histories = set()
    for spec in DISTRICTS:
        key, district, total, detailed = spec[:4]
        homes = make_district(spec, set(named), histories)
        people = sum(len(h[-1]) for h in homes)
        assert people + detailed == total
        households += len(homes)
        groups = [homes[i:i + VOLUME] for i in range(0, len(homes), VOLUME)]
        links = []
        for number, part in enumerate(groups, 1):
            render_volume(key, district, number, len(groups), part, search)
            links.append(f'<li><a href="{volume_name(key, number)}">{number}권 · {part[0][0]}–{part[-1][0]}</a></li>')
        sections.append(f'<details><summary>{district} · {len(homes):,}가구, {people:,}명</summary><ol>{"".join(links)}</ol></details>')
        print(f"{key}: {people} residents, {len(homes)} households, {len(groups)} volumes")
    ids = [row[1] for row in search]
    assert len(ids) == len(set(ids)) == 23954
    body = f"""<h1>로덴 가구별 주민 명부</h1>
<p>사고 전날 기준으로 로덴 상주민은 24,000명이다. <a href="../2026-11-28-roden-lives.html">상세 인물편</a>에 실린 로덴 주민 46명을 빼고, 이 대장에는 나머지 23,954명을 {households:,}가구로 묶었다. 방문자 린은 벨레스 주민이다.</p>
<p>각 가구 안에서 식구 관계, 성별, 나이, 생업, 외모와 짧은 개인사를 읽을 수 있다. 가구 번호는 동명이인을 구별하는 명부 번호다. 골목명과 가구 구성은 제작용 설정이며 한 건물에 여러 가구가 살 수 있다.</p>
<label for="query">이름·가구 번호·생업·골목 찾기</label> <input type="search" id="query"><p id="count" aria-live="polite"></p><ul id="matches"></ul>
<h2>구획별 대장</h2>{''.join(sections)}
<script type="application/json" id="names">{json.dumps(search, ensure_ascii=False, separators=(',', ':'))}</script>{SEARCH_JS}"""
    (OUT / "index.html").write_text(page("로덴 가구별 주민 명부", body), encoding="utf-8")
    print(f"Total: {len(ids)} residents, {households} households")


if __name__ == "__main__":
    main()
