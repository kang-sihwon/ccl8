// 숫자의 집계 대상과 판본을 사례별로 함께 보관한다.
const groups = ['소설', '비디오게임', '도시 비교', '테이블톱 RPG'];
const cases = [
  {
    id:'riverfarm', name:'리버팜', original:'Riverfarm', group:0, work:'pirateaba · The Wandering Inn', when:'3.12 E, 눈사태 시점 → 5.09 E',
    count:'주민 100명 조금 넘음', unit:'주민', space:'집들이 모인 강가와 숲 가장자리의 별채',
    note:'눈사태 뒤 가멜의 시점에서 서술자가 마을 주민 수를 밝힌다. 농가의 지붕과 집으로 돌아가는 길을 알던 사람이 눈 속에서 방향을 잃는다. 뒤에 라켄이 감지하는 천 명 넘는 사람은 이주민이 합류한 뒤 해당 일대에 있는 사람들이다.',
    links:[['3.12 E · 재난 당시 마을 인구','https://wanderinginn.com/2017/09/12/3-12-e/'],['5.09 E · 이주 이후의 범위','https://wanderinginn.com/2018/08/07/5-09-e/']]
  },
  {
    id:'windrest',name:'윈드레스트',original:'Windrest',group:0,work:'pirateaba · The Wandering Inn',when:'4.21 E · 이주 행렬 도착',
    count:'도착 뒤 수용 마을 인구 약 2배',unit:'사건',space:'짐수레와 당나귀가 지나온 마을 사이의 길',
    note:'리버팜에 도착한 행렬을 본 라켄은 마을 인구가 단숨에 두 배가 됐다고 느낀다. 도착 전후를 비교한 인물의 표현이다. 눈사태 당시의 리버팜 수치를 대입해 윈드레스트의 원래 주민 수를 역산하기는 어렵다.',
    links:[['4.21 E · 수레가 마을에 들어오는 장면','https://wanderinginn.com/2018/03/24/4-21-e/']]
  },
  {
    id:'bree',name:'브리',original:'Bree',group:0,work:'J. R. R. 톨킨 · 반지 원정대',when:'1권 9장 · 일행이 서문에 도착할 때',
    count:'인간의 돌집 약 100채',unit:'건물',space:'언덕 서사면 · 큰길을 감싸는 생울타리와 두 문',
    note:'언덕 아래로 난 길 위쪽에 인간의 돌집이 모이고, 호빗은 비탈의 굴집에 산다. 약 100이라는 수는 인간의 돌집만 센다. 밤에 닫히는 문과 여행자가 묵는 여관은 인근 네 마을보다 브리가 더 넓은 범위의 사람을 받는 이유를 보여 준다.',
    links:[['원작 장·저자 평면도 해설 · Tolkien Gateway','https://tolkiengateway.net/wiki/Bree']]
  },
  {
    id:'hobbiton',name:'호비튼',original:'Hobbiton',group:0,work:'J. R. R. 톨킨 · 호빗',when:'1937년 저자 삽화',
    count:'주민 총수 미상',unit:'공간',space:'언덕의 굴집 · 물가의 방앗간 · 다리 건너 길',
    note:'물 건너에서 언덕을 바라보면 지붕뿐 아니라 비탈 속의 문과 창이 보인다. 굴집이 많은 취락에서 지붕을 세면 사람이 사는 공간을 놓친다. 그림 속의 하천은 방앗간과 건너편 집들을 가르는 경계이기도 하다.',
    plate:['hobbiton','톨킨이 그린 호비튼. 강 건너에서 본 언덕과 방앗간','J. R. R. Tolkien · The Hill: Hobbiton-across-the Water, 1937'],
    links:[['톨킨 에스테이트 · 작가 수채화','https://www.tolkienestate.com/painting/the-hobbit/']]
  },
  {
    id:'laketown',name:'호수마을 에스가로스',original:'Lake-town',group:0,work:'J. R. R. 톨킨 · 호빗',when:'10장 A Warm Welcome · 스마우그의 습격 전',
    count:'주민 총수 미상',unit:'공간',space:'긴 호수 위 말뚝 시가지 · 육지로 나가는 긴 목교',
    note:'상업 취락 전체가 호숫가에서 떨어져 물 위에 서 있다. 화물을 싣는 배는 물에서 바로 안으로 들어올 수 있지만 육지로 걷는 사람은 긴 다리를 건넌다. 수면을 포함해 도판의 외곽을 재면 사람이 거주하는 바닥 넓이를 크게 잡게 된다.',
    links:[['10장 수록 저자 삽화 · Lake Town','https://tolkiengateway.net/wiki/File:J.R.R._Tolkien_-_Lake_Town.jpg'],['작품 내 지형 설명','https://www.tolkienfile.com/places/lake-town']]
  },
  {
    id:'carvahall',name:'카바홀',original:'Carvahall',group:0,work:'크리스토퍼 파올리니 · 에라곤 / 엘디스트',when:'엘디스트 29장 Conviction · 출발을 의논할 때',
    count:'피란 계획 약 300명',unit:'사건',space:'팔란카르 골짜기의 마을 중심과 바깥 농가',
    note:'로란이 마을 사람들을 이끌고 떠나기로 마음먹을 때 생각한 운송 규모다. 가게와 밭을 두고 떠날지 각 집이 결정한다. 준비한 인원과 실제 출발자, 평소 상주인구는 각각 다를 수 있다. 수백 명이 움직이면 먹거리와 짐승, 배편까지 이야기의 일부가 된다.',
    links:[['엘디스트 · 원서와 판본','https://www.penguinrandomhouse.com/books/126441/eldest-by-christopher-paolini/paperback/'],['29장 장면 안내','https://inheritance.fandom.com/wiki/Conviction'],['작가 자료실 · 비르기트 가족','https://www.paolini.net/2016/07/06/encyclopedia-alagaesia-birgit/']]
  },
  {
    id:'therinsford',name:'세린스포드',original:'Therinsford',group:0,work:'크리스토퍼 파올리니 · 에라곤',when:'16장 Therinsford · 다리를 건널 때',
    count:'카바홀보다 큰 마을',unit:'공간',space:'아노라강의 다리 · 농가 · 불규칙하게 늘어선 집',
    note:'에라곤은 다리에 닿기 전부터 농장과 길 위 발자국이 늘어나는 것을 본다. 집들의 방향도 고향과 다르다. 본문의 크기 비교에는 면적이나 인구의 배수가 없으므로 카바홀 피란 인원에서 이곳의 인구를 계산할 수 없다.',
    links:[['원서 · 출판사 독서 안내','https://www.penguinrandomhouse.com/books/126442/eragon-by-christopher-paolini/9780375826696/readers-guide'],['16장 · 도하와 말 구입','https://inheritance.fandom.com/wiki/Therinsford_(chapter)']]
  },
  {
    id:'emonds',name:'에몬즈 필드',original:'Emond’s Field',group:0,work:'로버트 조던 · 세계의 눈',when:'1장 · 벨 타인 축제 전날',
    count:'주민 총수 미상',unit:'공간',space:'외곽 농장 → 공동 풀밭과 샘 → 여관 · 하류 방앗간',
    note:'랜드와 아버지는 마을 밖 농장에서 물자를 싣고 들어온다. 축제 준비로 붐비는 공동터에는 외곽 농민도 모인다. 도착 장면 하나만으로 마을에 상주하는 사람을 세기 어렵지만, 샘에서 방앗간으로 이어지는 물길과 물자 반입 방향은 읽을 수 있다.',
    links:[['Tor / Macmillan · 1장 발췌','https://us.macmillan.com/books/9780312850098/theeyeoftheworld/'],['조던의 작업 노트 · 샘과 방앗간','https://www.theoryland.com/intvmain.php?i=1053']]
  },
  {
    id:'alders',name:'텐 올더스',original:'Ten Alders',group:0,work:'어슐러 K. 르 귄 · 어스시의 마법사',when:'1장 · 곤트섬 산마을에서 게드가 자라던 때',
    count:'대장장이의 아들 7명',unit:'가족',space:'골짜기 상단의 산길에 흩어진 집과 작업장',
    note:'게드는 여섯 형을 둔 막내다. 형들은 농사와 뱃일, 다른 마을의 대장일을 찾아 차례로 떠난다. 한 가족의 자녀 수와 당시 그 집에 함께 살던 인원부터 다르다. 섬 지도에서는 계곡의 위치를, 본문에서는 집 사이의 길을 읽는다.',
    plate:['gont','르 귄의 곤트섬 지도. 마을이 자리한 섬 전체를 그린 도판','Ursula K. Le Guin · Gont, 섬 위치도'],
    links:[['허가 발췌 · 1장','https://www.northcountrypublicradio.org/news/npr/4704490/excerpt-a-wizard-of-earthsea'],['작가 자료실 · 곤트섬 지도','https://www.ursulakleguin.com/maps-of-earthsea']]
  },
  {
    id:'solace',name:'솔러스',original:'Solace',group:0,work:'마거릿 와이스·트레이시 힉먼 · 가을 황혼의 용들',when:'창의 전쟁 도입부 · 소설과 동시대 모험서 DL1',
    count:'주민 약 500명',unit:'주민',space:'거목 위 집과 여관 · 땅에 있는 대장간',
    note:'소설 첫 장면에서 여관은 거대한 발렌우드 가지에 놓여 있고 대장간만 지상에 있다. 약 500명은 같은 세계의 모험서 DL1이 제시한 값이며 외곽 밭의 농민을 제외한다. 조감도에서 나무 밑동만 세면 여러 층의 거주 공간이 사라진다.',
    links:[['소설 첫 장면 · 출판본 견본','https://dragonlancenexus.com/wp-content/uploads/dragons-of-autumn-twilight-sample-chapter.pdf'],['DL1 Dragons of Despair · Solace 인구와 외곽 경계','https://d1vzi28wh99zvq.cloudfront.net/pdf_previews/16880-sample.pdf']]
  }
];

cases.push(
  {
    id:'riverwood',name:'리버우드',original:'Riverwood',group:1,work:'Bethesda · The Elder Scrolls V: Skyrim',when:'기본판 초반 · 탈출 경로에 따른 인물 제외',
    count:'이름 있는 인물 14명 · 거처 6곳',unit:'배치 인물',space:'제재소를 낀 강변 · 큰길의 대장간·잡화점·여관',
    note:'거처 표에서 알보르 집 3명, 펜달 집 1명, 거더 집 3명, 잡화점 2명, 여관 3명, 스벤 집 2명을 합산했다. 선택 경로의 해드버·랠로프, 개 스텀프, 나중에 오는 경비는 제외했다. 거처에는 영업장도 들어간다. 원화는 제재소 마을의 분위기를 보여 준다.',
    plate:['riverwood','레이 레더러의 리버우드 콘셉트 원화','Ray Lederer · TESV: Skyrim – Riverwood, 2013 공개'],
    links:[['UESP · 주민표와 조건 각주','https://en.uesp.net/wiki/Skyrim:Riverwood'],['게임 배경 아티스트 원화','https://raylederer.blogspot.com/2013/06/tesvskyrim-riverwood.html']]
  },
  {
    id:'rorik',name:'로릭스테드',original:'Rorikstead',group:1,work:'Bethesda · The Elder Scrolls V: Skyrim',when:'기본판 · 에릭의 모험 이전',
    count:'이름 있는 인물 9명 · 거처 4곳',unit:'배치 인물',space:'농가 두 곳과 여관·영주 집',
    note:'카우플롭 농가의 2명, 프로스트프루트 여관의 2명, 렘킬 농가의 아버지와 쌍둥이 3명, 로릭의 저택 2명을 합했다. 추가 콘텐츠 인물 할바르는 제외했다. 농장 일과 여관의 가족관계가 작은 배역표 안에서 겹친다.',
    plate:['rorik','경작지에서 주택가를 바라본 로릭스테드 화면','Skyrim · UESP 장소 기록'],
    links:[['UESP · 주민과 농가별 인물표','https://en.uesp.net/wiki/Skyrim:Rorikstead']]
  },
  {
    id:'orchard',name:'백색 과수원',original:'White Orchard village',group:1,work:'CD Projekt Red · 더 위쳐 3: 와일드 헌트',when:'프롤로그 · 닐프가드 점령기',
    count:'같은 이름의 권역에서 가장 큰 마을',unit:'공간',space:'여관과 도로가 만나는 취락 · 바깥의 주둔지와 전장',
    note:'권역 지도 전체가 마을과 같은 이름을 쓴다. 여관에서 마을 사람과 군인이 부딪히는 장면은 두 집단이 섞이는 장소를 보여 준다. 전장·방앗간·주둔지를 돌 때 만나는 인물을 모두 마을 주민으로 세면 권역과 취락이 뒤섞인다.',
    links:[['마을 표지와 권역 구분','https://witcher-games.fandom.com/wiki/White_Orchard_village'],['권역 지도 · 현장 위치 찾기','https://legacy.witcher3map.com/w/']]
  },
  {
    id:'midcopse',name:'미드콥스',original:'Midcopse',group:1,work:'CD Projekt Red · 더 위쳐 3: 와일드 헌트',when:'벨렌 · 게임 내 표지판',
    count:'화면의 건물 8동',unit:'건물',space:'밭을 낀 농촌 취락 · 전투가 비껴간 지역',
    note:'표지판은 벨렌의 비교적 큰 농촌 마을이라고 소개한다. 플레이 공간에서 건물 8동을 세었다는 값은 커뮤니티의 장소 조사에서 왔다. 작중 규모와 화면 배치가 한 장소에서 다르게 표현되는 예다. 마을이 전투를 피했어도 기근이 닥쳤다는 설정이 대사와 생활상을 뒷받침한다.',
    links:[['장소 조사 · 건물 8동과 표지판 전사','https://witcher.fandom.com/wiki/Midcopse'],['벨렌 지도 · 현장 표지판','https://mmo4ever.com/witcher/maps/velen-spitfire-bluff/']]
  },
  {
    id:'moon',name:'문헤이븐 · 황폐한 마을',original:'Moonhaven / Blighted Village',group:1,work:'Larian · 발더스 게이트 3',when:'1막 · 고블린 점거 뒤',
    count:'옛 주민 총수 미상',unit:'공간',space:'큰길과 풍차 · 남서쪽 학교 · 우물과 지하 공간',
    note:'옛 학교 안에 아이들 키를 표시한 자국과 실종 전단이 남아 있다. 플레이 시점에는 오우거 셋이 그곳에서 식사한다. 같은 건물의 흔적은 과거 주민을, 현재 조우는 점거자를 보여 준다. 폐허에 남은 적의 수로 옛 가구 수를 정할 수 없다.',
    plate:['moon','1막에 방문하는 문헤이븐의 폐허','Baldur’s Gate 3 · bg3.wiki 장소 화면'],
    links:[['현장·학교·우물·작중 대사','https://bg3.wiki/wiki/Blighted_Village']]
  },
  {
    id:'tarrey',name:'시자기 마을',original:'Tarrey Town',group:1,work:'Nintendo · 젤다의 전설 브레스 오브 더 와일드',when:'From the Ground Up · 개척부터 결혼식까지',
    count:'주인공이 모집하는 인물 4명',unit:'배치 인물',space:'개척지의 바위 → 원형으로 선 집과 점포 → 결혼식 터',
    note:'채굴·재봉·교역을 맡을 인물과 결혼식을 주관할 조라족을 차례로 부른다. 화면에는 동행 가족, 건축 책임자, 따로 들어오는 주민도 있다. 목재 납품과 인물 영입이 집·가게·숙소의 등장 시점을 결정한다.',
    plate:['tarrey','개척 뒤 집들이 원을 이루는 시자기 마을','Nintendo · 아칼라 지방 안내, 마을 전경'],
    links:[['Nintendo · 아칼라 지방, 개척 뒤 전경','https://www.nintendo.com/jp/zelda/botw/discovery/akkala.html'],['퀘스트 순서 · 모집 인물 명단','https://www.zeldadungeon.net/wiki/From_the_Ground_Up']]
  },
  {
    id:'tristram',name:'트리스트럼',original:'Tristram',group:1,work:'Blizzard North · 디아블로',when:'초대작 기본편 · 대성당에 들어가기 전후',
    count:'기본 인물 역할 8개 + 조건부 부상자 1명',unit:'배치 인물',space:'대성당 진입 전 수리·치료·감정을 맡기는 마을',
    note:'대장장이, 치유사, 여관 주인, 이야기꾼, 주정뱅이, 마녀, 여관 종업원, 소년을 포트의 기본 배치표에서 셌다. 부상자는 퀘스트 조건이 있고 소와 확장팩 인물은 제외했다. 매뉴얼에 따르면 이미 많은 주민이 떠났다. 마을을 반복해서 찾는 동안 이 여덟 역할이 남은 공동체의 얼굴이 된다.',
    links:[['Blizzard · 원작 매뉴얼','https://ftp.blizzard.com/pub/misc/Diablo.PDF'],['DevilutionX 1.5.4 · 재구현 배치표 784–798행','https://github.com/diasurgical/DevilutionX/blob/1.5.4/Source/towners.cpp#L784-L798']]
  },
  {
    id:'dallben',name:'달벤',original:'Dallben',group:1,work:'The Battle for Wesnoth · Liberty',when:'1.18.0 · 첫 시나리오 The Raid',
    count:'전장 파일 26열 × 32행',unit:'공간',space:'수비대가 막아야 할 마을 앞 접근로',
    note:'원본 맵 파일의 쉼표로 나뉜 지형 열과 행을 셌다. 가장자리 데이터와 마을까지 달려오는 고블린의 접근로를 포함한 크기다. 시나리오가 세는 것은 수비대와 습격자다. 병력이나 전장 칸 수에서 달벤의 민간인 수와 정주 면적을 환산할 수 없다.',
    plate:['dallben','리버티 도입부에 쓰인 변경 지대의 전경','The Battle for Wesnoth · Liberty의 도입 삽화 frontier, 광역 배경'],
    links:[['제작 저장소 · 시나리오와 패배 조건','https://github.com/wesnoth/wesnoth/blob/1.18.0/data/campaigns/Liberty/scenarios/01_The_Raid.cfg'],['원본 지형 행렬','https://raw.githubusercontent.com/wesnoth/wesnoth/1.18.0/data/campaigns/Liberty/maps/01_The_Raid.map'],['원본 삽화','https://github.com/wesnoth/wesnoth/blob/1.18.0/data/campaigns/Liberty/images/story/frontier.webp']]
  },
  {
    id:'archet',name:'아르쳇',original:'Archet',group:1,work:'The Lord of the Rings Online',when:'인간·호빗 도입부와 습격 이후',
    count:'습격 전후 2개 상태',unit:'사건',space:'마을길 · 여관 잔해 · 인근 사냥꾼 오두막',
    note:'도입부에서 오갔던 길로 돌아오면 불탄 여관 앞에서 다음 퀘스트를 받는다. 같은 장소의 전후 상태를 합쳐 건물이나 인물을 두 번 세지 않도록 한다. 브리 주변에 속하지만 브리의 돌집 수는 아르쳇에 적용되지 않는다.',
    links:[['도입부와 화재 뒤 현장','https://www.tentonhammer.com/articles/guide-to-bree-land'],['게임 퀘스트 전사 · 여관 잔해 앞','https://llb.sirhexalot.de/en/quest/70041381/']]
  },
  {
    id:'dali',name:'달리',original:'Dali',group:1,work:'Square · 파이널 판타지 IX',when:'초반 첫 방문 · 지하 시설을 발견할 때',
    count:'지상 마을과 지하 생산 시설',unit:'공간',space:'여관·풍차 주위의 짧은 길 · 아래로 이어지는 생산·적재 구역',
    note:'지상 화면은 한적한 농촌의 인상을 주지만, 풍차 아래에서 정체를 숨긴 생산 시설과 화물의 흐름이 드러난다. 두 층의 화면을 옆으로 펼쳐 합산하면 마을 폭을 잘못 읽게 된다. 노동력이나 주거 수용력을 정하려면 생산품과 주민 가구를 따로 확인해야 한다.',
    plate:['dali','초반 달리 마을의 첫 장소 화면과 연결 표식','Final Fantasy IX · OPATIL 현장 화면 1'],
    links:[['장소별 게임 화면과 이동 경로','https://ff9.opatil.com/eng/map/m09.html'],['마을과 지하 시설 안내','https://finalfantasy.fandom.com/wiki/Final_Fantasy_Wiki:Featured_Articles/Dali']]
  },
  {
    id:'novigrad',name:'노비그라드',original:'Novigrad',group:2,work:'CD Projekt Red · 더 위쳐 3: 와일드 헌트',when:'게임 배경에 대한 현장 취재',
    count:'설정 인구 30,000명 · 매체 보도',unit:'주민',space:'항구 · 상업 광장 · 성벽 안팎의 시가지',
    note:'PC Gamer의 도시 답사 기사가 설정 인구를 3만 명으로 소개한다. 제작사가 게임에서 동시에 생성한 군중의 수로 발표한 값은 아니다. 항구의 짐꾼, 광장의 상인, 경비와 행인이 서로 다른 구역에서 보이는 방식을 로덴과 비교할 수 있다.',
    plate:['novigrad','노비그라드의 항구와 시가지를 담은 게임 공략 지도','Gamepressure · The Witcher 3, Novigrad M6'],
    links:[['PC Gamer · 인구 출처와 도시 답사','https://www.pcgamer.com/faces-of-novigrad-a-closer-look-at-the-witcher-3s-biggest-city/'],['게임 지도 M6','https://www.gamepressure.com/thewitcher3/map-of-important-locations-m6/z67816']]
  },
  {
    id:'daggerfall',name:'대거폴',original:'Daggerfall',group:2,work:'Bethesda · The Elder Scrolls II: Daggerfall',when:'작중 역사서에 기록된 제3시대 401년',
    count:'작중 인구조사 110,000명 초과',unit:'주민',space:'도시국가의 수도 · 도시 간 규모 비교',
    note:'책 A History of Daggerfall의 저자는 인구조사 기록을 인용하며 센티널·웨이레스트와 시세를 비교한다. 게임 시작 시점의 거리 행인 수와 조사 연도는 다르다. 큰 수치가 나와도 조사 대상이 도시인지 왕국인지 문맥부터 확인할 사례다.',
    links:[['게임 속 역사서 전사','https://elderscrolls.fandom.com/wiki/A_History_of_Daggerfall'],['다른 판독본 · 마지막 인구조사 단락','https://gamefaqs.gamespot.com/pc/29065-the-elder-scrolls-chapter-ii-daggerfall/faqs/81144/a-history-of-daggerfall']]
  },
  {
    id:'haven',name:'이민자의 마을',original:'Immigrant Town / The Haven',group:1,work:'Enix · 드래곤 퀘스트 VII',when:'초대 PlayStation판 · 이민 모집',
    count:'입주민 최대 40명',unit:'운영 인구',space:'빈터에서 상점·교회·은행이 생기는 정착지',
    note:'모집한 이민자가 25~29명이면 교회가, 30~34명이면 은행이 생긴다. 35~40명 구간에는 상인·성직자 등 누가 왔느냐에 따라 마을의 최종 모습이 달라진다. 40명은 이 판본에서 플레이어가 채울 수 있는 상한이다.',
    links:[['PS판 · 40명 상한과 모집 조건','https://gamefaqs.gamespot.com/ps/197152-dragon-warrior-vii/faqs/9100'],['주민 수 구간별 건물 변화','https://www.neoseeker.com/dragon-quest-vii/faqs/49509-dragon-warrior-vii-immigrants.html']]
  },
  {
    id:'faerie',name:'요정 마을',original:'Faerie Village',group:1,work:'Capcom · 브레스 오브 파이어 III',when:'마을 운영을 시작한 뒤',
    count:'요정 주민 최대 20명',unit:'운영 인구',space:'식량을 얻는 사냥터 · 개간지와 새로 지을 집',
    note:'요정에게 사냥·개간·건축을 배분한다. 먹거리가 넉넉하면 새 요정이 태어나고, 사냥 인력을 작업장에 너무 많이 돌리면 식량이 줄어든다. 같은 20명이라도 먹을 것을 마련하는 인원이 얼마나 되는지에 따라 운영 가능한 가게가 달라진다.',
    links:[['플레이 기록 · 요정 인구 상한','https://gamefaqs.gamespot.com/ps/196817-breath-of-fire-iii/faqs/3739'],['업무·식량·출생 조건','https://strategywiki.org/wiki/Breath_of_Fire_III/Faeries']]
  },
  {
    id:'norende',name:'노르엔데',original:'Norende',group:1,work:'Square Enix · 브레이블리 디폴트',when:'Nintendo 3DS판 · 재건 메뉴',
    count:'플레이 기록마다 달라지는 복구 인구',unit:'운영 인구',space:'대균열이 삼킨 고향 · 장애물을 치우고 시설을 복구하는 지도',
    note:'공식 설명서는 재건 화면의 총인구를 통신으로 만난 사람 수로 정의한다. 실제로 작업 중인 인원은 옆의 별도 값이다. 재난 이전 마을의 주민 수나 어느 한 플레이어의 결과를 보편적인 설정 인구로 쓸 수 없다.',
    links:[['Nintendo 3DS 공식 전자설명서 · Norende Village, Worker count/total population','https://www.nintendo.com/eu/media/downloads/games_8/emanuals/nintendo_3ds_2/bravely_default/ElectronicManual_BravelyDefault_EN.pdf']]
  }
);

cases.push(
  {
    id:'willow',name:'윌로쇼어',original:'Willowshore',group:3,work:'Paizo · Season of Ghosts',when:'리마스터판 플레이어 가이드 · 모험 시작 전',
    count:'주민 225명',unit:'주민',space:'강 양안의 벌목 취락 · 다리와 강 밖의 작업지',
    note:'공식 정착지 표는 외딴 벌목 마을에 225명이 산다고 기록한다. 두 기슭의 집줄을 먼저 보고 다리에서 외곽의 벌목 터까지 시선을 옮겨 보자. 마구간은 동물을 치료하고 사역용 가축을 빌려 준다. 떨어져 있는 일터도 주민의 생업에 속한다.',
    plate:['willow','윌로쇼어의 벌목 취락과 여러 갈래의 강','Paizo · Season of Ghosts Player’s Guide, p.16 지도 부분'],
    metric:{metres:365.76,pixels:152,focus:[.42,.59],label:'1,200피트',image:[1206,790],a:[145,576],b:[558,452],route:'W2 마구간 → W14 댐'},
    links:[['p.17 · 인구와 통치','https://downloads.paizo.com/2026SeasonofGhosts_PlayersGuide.pdf#page=17'],['p.16 · 축척 지도와 집 번호','https://downloads.paizo.com/2026SeasonofGhosts_PlayersGuide.pdf#page=16']]
  },
  {
    id:'otari',name:'오타리',original:'Otari',group:3,work:'Paizo · Abomination Vaults',when:'플레이어 가이드 · 모험 시작 전',
    count:'주민 1,240명',unit:'주민',space:'제재 설비 → 통나무 운반 수로 → 항구',
    note:'수차가 제재 설비를 움직이고 긴 운반 수로가 목재를 항만까지 보낸다. 지도 번호 1·2를 먼저 찾으면 주민 일자리와 항구가 왜 이웃해야 하는지 읽힌다. 번호가 붙은 16곳은 모험 안내의 주요 장소다. 주택 16채나 주민 16명을 뜻하지 않는다.',
    plate:['otari','오타리 항구와 물레바퀴, 목재 운반 시설','Paizo · Abomination Vaults Player’s Guide, p.6 지도 부분'],
    metric:{metres:121.92,pixels:140,focus:[.48,.54],label:'400피트',image:[904,645],a:[638,278],b:[590,401],route:'1번 수차 → 2번 목재 운반 수로 표식'},
    links:[['p.7 · 인구와 생업','https://downloads.paizo.com/AbominationVaults_PlayersGuide.pdf#page=7'],['p.6 · 축척 지도·장소 1~16','https://downloads.paizo.com/AbominationVaults_PlayersGuide.pdf#page=6']]
  },
  {
    id:'sand',name:'샌드포인트',original:'Sandpoint',group:3,work:'Paizo · Seven Dooms for Sandpoint',when:'플레이어 가이드 · 해안 소도시',
    count:'주민 1,240명',unit:'주민',space:'해안과 강 사이의 본시가 · 다리 건너 땅',
    note:'오타리와 설정 인구가 같지만 해안선과 강이 길의 방향을 다르게 정한다. 태너의 다리에서 해안 쪽 도로로 돌아보면 통과 교통과 항구에 모이는 짐의 동선이 갈라진다. 지도에는 건너편 저택과 외곽도 들어 있다.',
    plate:['sand','샌드포인트의 거리·항구·두 다리를 담은 지도','Paizo · Seven Dooms for Sandpoint Player’s Guide, p.21'],
    metric:{metres:91.44,pixels:176,focus:[.56,.46],label:'300피트',image:[1206,1566],a:[1008,520],b:[950,1092],route:'태너의 다리 → 샌드포인트 다리'},
    links:[['p.17 · 정착지 인구표','https://downloads.paizo.com/SevenDoomsforSandpoint_PlayersGuide.pdf#page=17'],['p.21 · 축척 도면','https://downloads.paizo.com/SevenDoomsforSandpoint_PlayersGuide.pdf#page=21']]
  },
  {
    id:'phandalin',name:'판달린',original:'Phandalin',group:3,work:'Wizards of the Coast · Dragon of Icespire Peak',when:'옛 폐허 위에 취락이 다시 선 시점',
    count:'통나무 건물 40~50동',unit:'건물',space:'농지와 광산을 잇는 길 · 새 집과 폐허의 경계',
    note:'공식 안내의 40~50동에는 살림집과 상점이 함께 들어간다. 더 넓은 옛 시가지의 무너진 돌담도 한 화면에 있어, 남은 기초를 전부 세면 현재 마을을 크게 잡는다. 농부와 대장장이·상인·탐광자가 같은 길을 쓰는 이유가 마을 입지에서 드러난다.',
    plate:['phandalin','마이크 슐리가 그린 판달린 주택·농장·도로 배치','Mike Schley · Phandalin, 기존 지도 자료집 수록 도판'],
    links:[['공식 건물 수·도판 · D&D Beyond','https://www.dndbeyond.com/posts/1515-visit-phandalin-d-ds-most-popular-starter-town'],['기존 자료집 · 원도와 부분도','../MedievalSettlementMapReferences/index.html#artschphandalin']]
  }
);
