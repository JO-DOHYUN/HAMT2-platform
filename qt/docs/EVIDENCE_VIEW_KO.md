# EVIDENCE_VIEW_KO

## 목적
같은 시간축에서 아래 4개를 겹쳐 보여 원인 분리가 가능해야 한다.
- Sensor Direct
- Controller CAN
- Command TX
- Board Event

## 판별 예시
- direct 정상 / controller만 이상 -> 제어기 보고값 또는 decode 문제 가능성
- direct부터 이상 -> 실제 센서/배선/노이즈 가능성
- command 직후만 이상 -> 제어 영향 가능성
- board event 동반 -> 관측계 한계 가능성

## 필수 표시
- source badge
- drop/overflow marker
- time-sync 상태
- divergence marker
