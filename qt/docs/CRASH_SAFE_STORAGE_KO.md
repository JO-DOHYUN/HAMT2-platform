# CRASH_SAFE_STORAGE_KO

## 필수
- 기록 중 상태를 operator가 명확히 봐야 한다
- `.part`가 남아도 복구 또는 incomplete 판별 가능해야 한다
- index 손상 시 stream 스캔으로 재생성 가능해야 한다
- meta는 마지막 정상본이 남도록 atomic save 사용
