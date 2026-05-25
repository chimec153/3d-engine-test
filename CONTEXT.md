# CONTEXT

## 용어 (Glossary)

무기 제작·전투 도메인의 핵심 용어. 코드가 역사적으로 `Weapon` 을 "조합
결과물"과 "스테이지 발사" 양쪽에 모두 써서 의미가 겹치므로, 대화·문서는
아래 용어로 구분한다. (코드 식별자 리네임은 아직 하지 않음 — 의미 매핑만.)

- **부품 (Part)** — 무기를 조합할 때 넣는 속성 한 종류. 6개 범주:
  발사 원점(SpawnOrigin), 이동 방식(MovementType), 발사 방식(FireMode),
  피격 효과(OnHitEvent), 투사체(ProjectileShape), 레벨업(LevelUpField).
  · 코드: `WeaponData.h` 의 enum들, `WeaponCombiner` 의 6개 슬롯 범주.

- **부품 카드 (Part Card)** — 부품 하나를 나타내는 카드. 조합 씬
  인벤토리의 네모 아이콘으로, 더블클릭하면 같은 범주의 슬롯에 장착된다.
  · 코드: `WeaponCombiner` 의 `AttrDef` / `kPalette` / 인벤토리 아이콘.

- **무기 카드 (Weapon Card)** — 부품 6종을 조합해 만든 무기 정의(레시피).
  제작 목록(로드아웃, 최대 10개 장착)에 쌓이고, 스테이지 레벨업 화면에
  선택지로 제시된다.
  · 코드: `WeaponDef` / `WeaponDatabase` 의 제작 레지스트리 / `weapons.csv`.

- **무기 (Weapon)** — 스테이지에서 플레이어 슬롯에 장착돼 실제로 총알을
  발사하는 인스턴스. 무기 카드를 레벨업에서 선택하면 얻는다(슬롯 최대 6).
  · 코드: `Player::WeaponSlot` / `m_vecWeaponSlots` / `WeaponHUD`.

### 관계
부품(Part) 6개를 조합 → **무기 카드(Weapon Card)** 제작 → 스테이지
레벨업에서 무기 카드 선택 → **무기(Weapon)** 획득 → 총알 발사.
