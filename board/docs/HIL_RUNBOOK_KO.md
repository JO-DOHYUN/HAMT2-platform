# HIL_RUNBOOK_KO

## 최소 검증 세트
1. CAN flood 중 hidden drop 없는지
2. DBS60E equivalent encoder pulse injection 시 miss/overflow/index fault가 가시화 되는지
3. ADC burst 동시 사용 시 CAN path 보호되는지
4. reconnect / lease timeout / estop 전이 일관성 있는지
5. power-loss 후 session 복구 가능한지
6. 2h 이상 soak 에서 drift/overflow 양상 보이는지

## 통과 기준
- 손실이 없어야 이상적이지만,
- 손실이 생기면 반드시 lane/time/counter/event로 드러나야 한다.

## 구동 엔코더 추가 기준
- A/B는 `PC6`/`PC7`의 TIM3 encoder mode로 검증한다.
- Z index는 `PA8` EXTI timestamp와 TIM3 extended count snapshot이 함께 남아야 한다.
- 2048 PPR, 6000 rpm 상당인 204.8 kHz/channel, 819.2 kedge/s x4 조건을 통과 기준으로 둔다.
- 300 kHz/channel 근접 입력에서는 정상 범위 초과 event를 남기고, 카운터 포화나 조용한 wrap을 허용하지 않는다.
- field power loss, receiver fault, cable open/short 추정 상태는 `BOARD_HEALTH` 또는 `BOARD_EVENT`로 드러나야 한다.
