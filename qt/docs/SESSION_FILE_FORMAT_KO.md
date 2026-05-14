# SESSION_FILE_FORMAT_KO

## 권장 세트
- `capture.stream.part` : 원문 typed record stream
- `capture.index.part` : sparse time->offset index
- `session.meta.json` : capability, profile, time map, session state
- `events.jsonl` : operator note / control / board event side log

## 원칙
- 원문 stream은 append-only
- finalize 이전 중단도 복구 가능해야 함
- meta는 atomic save
- replay는 항상 stream 기준으로 재구성
